#ifndef __COMM_NB_H__
#define __COMM_NB_H__

/*
 * Non-Blocking COMM Transaction Engine — ADBMS1818
 * ================================================
 * Drives the WRCOMM → STCOMM → (RDCOMM → parse) sequence as an event-driven
 * state machine so the CPU is not stuck inside SPI transfers.
 *
 * Portable core: this file and commNb.c contain NO platform/HAL code.  All
 * SPI and CS access goes through a CommNbBackend the caller supplies.  A
 * ready-made STM32 backend lives in prog/src/wrapperNb.c (commNb_stm32_init).
 *
 * ── How a transaction runs ──────────────────────────────────────────────────
 *
 *   1. The caller fills a CommNbCtx (ic array, count, two callbacks).
 *   2. commNb_Start(ctx) invokes frame_cb(frame 0) to load ic->comm, then
 *      kicks off the first WRCOMM.
 *   3. As each SPI transfer completes, the platform calls commNb_OnTxDone()
 *      / commNb_OnRxDone() from its ISR (or, in blocking mode, the backend
 *      calls them inline).  The state machine advances:
 *
 *        WRCOMM ─tx─► STCOMM ─tx─► [needs_rdcomm?]
 *                                    │yes              │no
 *                                    ▼                 ▼
 *                              RDCOMM_TX ─tx─►     next frame / done
 *                              RDCOMM_RX ─rx─► parse ─► next frame / done
 *
 *   4. After the last frame, done_cb(status) fires and the engine returns
 *      to idle.
 *
 * ── Multi-frame ─────────────────────────────────────────────────────────────
 * frame_cb is called once per frame.  It fills ic->comm using the
 * comm_i2c_* / comm_spi_* helpers (commHelper.h) and returns how many frames
 * still remain AFTER this one (0 = this was the last).  Because the callback
 * runs each time, read continuations can depend on bytes parsed from the
 * previous frame — exactly like the blocking template loop.
 *
 * ── Re-entrancy / blocking ──────────────────────────────────────────────────
 * OnTxDone/OnRxDone funnel through an internal dispatcher guarded against
 * re-entry, so a blocking backend that calls them inline runs the whole
 * chain to completion inside commNb_Start without recursion.  IT/DMA backends
 * call them from the ISR and the chain advances one step per interrupt.
 */

#include <stdint.h>
#include "dataSetList.h"   /* cell_asic_, comm_ */

#ifdef __cplusplus
extern "C" {
#endif

/* Max ICs the static scratch buffers are sized for; override with -D if your
 * chain is longer than the default. */
#ifndef COMM_NB_MAX_IC
#define COMM_NB_MAX_IC 8
#endif

/* ── Smart auto-wake thresholds (ms) ──────────────────────────────────────────
 * Datasheet-grounded, using the conservative MINIMUM timeouts so we wake a
 * touch eagerly rather than risk talking to an asleep chip:
 *   < IDLE_MS         → chain still active, no wake (hot path, zero cost)
 *   IDLE_MS..SLEEP_MS → isoSPI port idle, core in standby → cheap tREADY wake
 *   ≥ SLEEP_MS        → core asleep → full tWAKE wake (per device)
 * Auto-wake is active only when the backend supplies now_ms(); otherwise the
 * caller owns wake-up (legacy behaviour). */
#ifndef COMM_NB_IDLE_MS
#define COMM_NB_IDLE_MS    4u      /* > tIDLE min 4.3 ms */
#endif
#ifndef COMM_NB_SLEEP_MS
#define COMM_NB_SLEEP_MS   1800u   /* > tWDT  min 1.8 s  */
#endif

/* ── Completion status ────────────────────────────────────────────────────── */
typedef enum {
    COMM_NB_OK  = 0,
    COMM_NB_ERR = 1     /* SPI error reported via commNb_OnError()           */
} CommNbStatus;

/* ── Return code from commNb_Start ────────────────────────────────────────── */
typedef enum {
    COMM_NB_STARTED = 0,   /* transaction kicked off                         */
    COMM_NB_BUSY    = 1,   /* another transaction already running — rejected */
    COMM_NB_NO_BACKEND = 2 /* commNb_SetBackend() never called               */
} CommNbRet;

/* ── Platform backend ─────────────────────────────────────────────────────────
 * cs_low/cs_high : drive the ADBMS chip-select GPIO.
 * tx_async       : start sending len bytes.  Non-blocking backends return
 *                  immediately and later call commNb_OnTxDone() from the ISR.
 *                  A blocking backend transfers then calls commNb_OnTxDone()
 *                  before returning.  Return 0 on success.
 * rx_async       : same contract, completion signalled by commNb_OnRxDone().
 * delay_ns       : OPTIONAL (may be NULL).  Honour the inter-command CSB-high
 *                  gap (t5/t6).  If NULL the engine relies on natural latency.
 * now_ms         : OPTIONAL (may be NULL).  Free-running millisecond counter
 *                  (HAL_GetTick / millis / esp_timer).  When supplied, the
 *                  engine tracks idle time and auto-wakes the chain as needed
 *                  before a transaction.  NULL → caller owns wake-up. */
typedef struct {
    void     (*cs_low)(void);
    void     (*cs_high)(void);
    int      (*tx_async)(const uint8_t *data, uint16_t len);
    int      (*rx_async)(uint8_t *data, uint16_t len);
    void     (*delay_ns)(uint32_t ns);
    uint32_t (*now_ms)(void);
} CommNbBackend;

/* ── Per-frame builder callback ───────────────────────────────────────────────
 * Called once per frame (frame_index 0,1,2,…) to populate ic[*].comm.
 *   ic           : the IC array from the context
 *   tIC          : number of ICs
 *   frame_index  : 0-based index of the frame being built
 *   needs_rdcomm : OUT — set 1 if this frame must RDCOMM+parse to recover RX
 *                  bytes before the next frame; set 0 for pure writes
 *   user_data    : opaque pointer from the context
 * Returns: number of frames remaining AFTER this one (0 = last frame).
 *
 * Runs in ISR context for non-blocking backends — keep it short, no blocking
 * calls.  The comm_i2c_ / comm_spi_ helpers are safe (struct fills only). */
typedef uint8_t (*CommNbFrameCb)(cell_asic_ *ic, uint8_t tIC,
                                 uint8_t frame_index, uint8_t *needs_rdcomm,
                                 void *user_data);

/* ── Completion callback ──────────────────────────────────────────────────── */
typedef void (*CommNbDoneCb)(CommNbStatus status, void *user_data);

/* ── Transaction context (caller-allocated, must outlive the transfer) ─────── */
typedef struct {
    cell_asic_   *ic;
    uint8_t       tIC;
    CommNbFrameCb frame_cb;
    CommNbDoneCb  done_cb;    /* may be NULL */
    void         *user_data;
} CommNbCtx;

/* ── API ──────────────────────────────────────────────────────────────────── */

/* Register the platform backend.  Call once at init (the STM32 adapter does
 * this for you inside commNb_stm32_init). */
void      commNb_SetBackend(const CommNbBackend *backend);

/* Start a transaction.  ctx must remain valid until done_cb fires.
 * Rejected with COMM_NB_BUSY if one is already running. */
CommNbRet commNb_Start(CommNbCtx *ctx);

/* 1 while a transaction is in progress, 0 when idle. */
uint8_t   commNb_IsBusy(void);

/* Mark the bus as just-active so the idle timer restarts.  Call this from any
 * OTHER code that talks to the chain (blocking reads, config writes) so the
 * auto-wake logic doesn't needlessly wake an already-awake chip. */
void      commNb_NoteActivity(void);

/* Completion hooks — wire these to your SPI ISR callbacks (or call inline
 * from a blocking backend). */
void      commNb_OnTxDone(void);
void      commNb_OnRxDone(void);
void      commNb_OnError(void);

#ifdef __cplusplus
}
#endif

#endif /* __COMM_NB_H__ */
