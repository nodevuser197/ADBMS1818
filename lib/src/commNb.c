#include "commNb.h"
#include "parseCreate.h"   /* createComm, parseComm — HAL-free        */
#include "adbmsTiming.h"   /* adbms_cmdGap_ns, wake timing constants  */

/* Pulled in directly rather than via genericType.h, which would drag in the
 * STM32 wrapper/HAL chain and break this file's platform independence. */
extern uint16_t Pec15_Calc(uint8_t len, uint8_t *data);
#define NB_TX_DATA 6   /* COMM data bytes per IC (matches TX_DATA)        */
#define NB_RX_DATA 8   /* read register bytes per IC (6 data + 2 PEC)     */

/* COMM command codes */
#define NB_WRCOMM_0 0x07
#define NB_WRCOMM_1 0x21
#define NB_RDCOMM_0 0x07
#define NB_RDCOMM_1 0x22
#define NB_STCOMM_0 0x07
#define NB_STCOMM_1 0x23

/* ── State machine ────────────────────────────────────────────────────────── */

typedef enum {
    ST_IDLE = 0,
    ST_WRCOMM,      /* WRCOMM packet in flight                */
    ST_STCOMM,      /* STCOMM (cmd + 9 dummy) in flight        */
    ST_RDCOMM_TX,   /* RDCOMM command in flight (CS held low)  */
    ST_RDCOMM_RX    /* receiving read-back bytes               */
} nb_state_t;

typedef enum { EV_NONE = 0, EV_TX, EV_RX, EV_ERR } nb_event_t;

static const CommNbBackend *s_be;
static CommNbCtx           *s_ctx;

static volatile nb_state_t  s_state       = ST_IDLE;
static volatile nb_event_t  s_pending     = EV_NONE;
static volatile uint8_t     s_dispatching = 0;

static uint8_t s_frame_index;
static uint8_t s_remaining;     /* frames left AFTER the current one */
static uint8_t s_needs_rdcomm;

/* Idle tracking for smart auto-wake. */
static uint32_t s_last_activity_ms;
static uint8_t  s_ever_active;  /* 0 until first wake — forces cold wake */

/* Static scratch — must outlive async transfers, so never stack-allocated. */
static uint8_t s_wrcomm[4 + (NB_TX_DATA + 2) * COMM_NB_MAX_IC];
static uint8_t s_stcomm[13];                       /* 4 cmd + 9 dummy        */
static uint8_t s_rdcomm[4];
static uint8_t s_rxbuf[NB_RX_DATA * COMM_NB_MAX_IC];

/* ── Helpers ──────────────────────────────────────────────────────────────── */

static uint8_t clamp_ic(uint8_t tIC)
{
    return (tIC > COMM_NB_MAX_IC) ? COMM_NB_MAX_IC : tIC;
}

static void build_cmd4(uint8_t *buf, uint8_t b0, uint8_t b1)
{
    buf[0] = b0;
    buf[1] = b1;
    uint16_t pec = Pec15_Calc(2, buf);
    buf[2] = (uint8_t)(pec >> 8);
    buf[3] = (uint8_t)pec;
}

/* Pack ic[*].comm → wire buffer: WRCOMM cmd + per-IC (6 data + 2 PEC).
 * ICs are emitted last-first to match the daisy-chain write order. */
static uint16_t build_wrcomm(void)
{
    uint8_t  tIC = clamp_ic(s_ctx->tIC);
    uint8_t  copy[NB_TX_DATA];
    uint16_t idx = 4;

    createComm(tIC, s_ctx->ic);
    build_cmd4(s_wrcomm, NB_WRCOMM_0, NB_WRCOMM_1);

    for (uint8_t cur = tIC; cur > 0; cur--) {
        const uint8_t *src = s_ctx->ic[cur - 1].ic_register.tx_data;
        for (uint8_t b = 0; b < NB_TX_DATA; b++) {
            s_wrcomm[idx++] = src[b];
            copy[b]         = src[b];
        }
        uint16_t pec = Pec15_Calc(NB_TX_DATA, copy);
        s_wrcomm[idx++] = (uint8_t)(pec >> 8);
        s_wrcomm[idx++] = (uint8_t)pec;
    }
    return idx;
}

static void build_stcomm(void)
{
    build_cmd4(s_stcomm, NB_STCOMM_0, NB_STCOMM_1);
    for (uint8_t i = 0; i < 9; i++)
        s_stcomm[4 + i] = 0xFF;   /* MOSI don't-care during 72-clock execute */
}

/* Strip PEC from each 8-byte read register, flag PEC errors, then unpack. */
static void parse_rx(void)
{
    uint8_t tIC = clamp_ic(s_ctx->tIC);
    uint8_t stripped[NB_TX_DATA * COMM_NB_MAX_IC];

    for (uint8_t cur = 0; cur < tIC; cur++) {
        uint8_t *reg = &s_rxbuf[cur * NB_RX_DATA];
        for (uint8_t b = 0; b < NB_TX_DATA; b++)
            stripped[cur * NB_TX_DATA + b] = reg[b];

        uint16_t rx_pec   = ((uint16_t)reg[NB_TX_DATA] << 8) | reg[NB_TX_DATA + 1];
        uint16_t calc_pec = Pec15_Calc(NB_TX_DATA, reg);
        s_ctx->ic[cur].cccrc.comm_pec = (rx_pec != calc_pec) ? 1 : 0;
    }
    parseComm(tIC, s_ctx->ic, stripped);
}

/* Raise CS and honour the inter-command CSB-high gap (t5/t6). */
static void cmd_gap(void)
{
    s_be->cs_high();
    if (s_be->delay_ns)
        s_be->delay_ns(adbms_cmdGap_ns(s_ctx->tIC));
}

/* Issue wake pulses and hold off long enough for the chain to be ready.
 * COMM is a GPIO-level I2C/SPI bridge and does NOT use the ADC reference, so
 * a cold wake needs only tWAKE (regulator/isoSPI) — tREFUP is not required. */
static void do_wake(uint8_t full)
{
    uint8_t  tIC    = clamp_ic(s_ctx->tIC);
    uint32_t per_ns = full ? (ADBMS_TWAKE_US * 1000u)    /* sleep → standby */
                           : (ADBMS_TREADY_US * 1000u);  /* idle  → ready   */

    for (uint8_t i = 0; i < tIC; i++) {
        s_be->cs_low();
        if (s_be->delay_ns) s_be->delay_ns(per_ns);
        s_be->cs_high();
        if (s_be->delay_ns) s_be->delay_ns(adbms_cmdGap_ns(tIC));
    }
}

/* Decide whether the chain needs waking based on elapsed idle time. */
static void auto_wake(void)
{
    if (!s_be->now_ms)            /* no time source → caller owns wake-up */
        return;

    uint32_t now     = s_be->now_ms();
    uint32_t elapsed = now - s_last_activity_ms;   /* wraps correctly */

    if (!s_ever_active || elapsed >= COMM_NB_SLEEP_MS)
        do_wake(1);              /* cold / slept → full tWAKE wake */
    else if (elapsed >= COMM_NB_IDLE_MS)
        do_wake(0);              /* port idle → cheap tREADY wake  */
    /* else: still active, no wake */

    s_ever_active = 1;
}

static void note_activity(void)
{
    if (s_be && s_be->now_ms)
        s_last_activity_ms = s_be->now_ms();
    s_ever_active = 1;
}

static void finish(CommNbStatus status)
{
    s_state = ST_IDLE;
    note_activity();
    if (s_ctx && s_ctx->done_cb)
        s_ctx->done_cb(status, s_ctx->user_data);
}

static void start_wrcomm(void)
{
    uint16_t len = build_wrcomm();
    s_be->cs_low();
    s_state = ST_WRCOMM;
    s_be->tx_async(s_wrcomm, len);
}

static void advance_frame(void)
{
    if (s_remaining == 0) {
        finish(COMM_NB_OK);
        return;
    }
    s_frame_index++;
    s_needs_rdcomm = 0;
    s_remaining = s_ctx->frame_cb(s_ctx->ic, s_ctx->tIC,
                                  s_frame_index, &s_needs_rdcomm,
                                  s_ctx->user_data);
    start_wrcomm();
}

/* ── Event handling ───────────────────────────────────────────────────────── */

static void handle_event(nb_event_t ev)
{
    if (ev == EV_ERR) {
        s_be->cs_high();
        finish(COMM_NB_ERR);
        return;
    }

    switch (s_state) {
        case ST_WRCOMM:                 /* WRCOMM sent → run STCOMM           */
            cmd_gap();
            build_stcomm();
            s_be->cs_low();
            s_state = ST_STCOMM;
            s_be->tx_async(s_stcomm, sizeof s_stcomm);
            break;

        case ST_STCOMM:                 /* STCOMM done → read back or advance  */
            cmd_gap();
            if (s_needs_rdcomm) {
                build_cmd4(s_rdcomm, NB_RDCOMM_0, NB_RDCOMM_1);
                s_be->cs_low();
                s_state = ST_RDCOMM_TX;
                s_be->tx_async(s_rdcomm, sizeof s_rdcomm);
            } else {
                advance_frame();
            }
            break;

        case ST_RDCOMM_TX:              /* RDCOMM cmd sent → clock in data     */
            s_state = ST_RDCOMM_RX;     /* CS stays low for the read           */
            s_be->rx_async(s_rxbuf, (uint16_t)(NB_RX_DATA * clamp_ic(s_ctx->tIC)));
            break;

        case ST_RDCOMM_RX:              /* data received → parse, advance      */
            cmd_gap();
            parse_rx();
            advance_frame();
            break;

        default:
            break;
    }
}

/* Re-entrancy-guarded dispatcher. A blocking backend calls OnTxDone/OnRxDone
 * inline; the guard turns that into an iterative loop instead of recursion. */
static void dispatch(void)
{
    if (s_dispatching)
        return;
    s_dispatching = 1;
    while (s_pending != EV_NONE) {
        nb_event_t ev = s_pending;
        s_pending = EV_NONE;
        handle_event(ev);
    }
    s_dispatching = 0;
}

/* ── Public API ───────────────────────────────────────────────────────────── */

void commNb_SetBackend(const CommNbBackend *backend)
{
    s_be = backend;
}

CommNbRet commNb_Start(CommNbCtx *ctx)
{
    if (!s_be || !s_be->cs_low || !s_be->cs_high ||
        !s_be->tx_async || !s_be->rx_async)
        return COMM_NB_NO_BACKEND;

    if (s_state != ST_IDLE)
        return COMM_NB_BUSY;

    s_ctx          = ctx;
    s_frame_index  = 0;
    s_needs_rdcomm = 0;

    auto_wake();             /* wake the chain only if it has gone idle/asleep */

    s_remaining    = ctx->frame_cb(ctx->ic, ctx->tIC, 0,
                                   &s_needs_rdcomm, ctx->user_data);
    start_wrcomm();          /* blocking backend runs the whole chain here  */
    return COMM_NB_STARTED;
}

uint8_t commNb_IsBusy(void)
{
    return (s_state != ST_IDLE) ? 1u : 0u;
}

void commNb_NoteActivity(void)
{
    note_activity();
}

void commNb_OnTxDone(void) { s_pending = EV_TX;  dispatch(); }
void commNb_OnRxDone(void) { s_pending = EV_RX;  dispatch(); }
void commNb_OnError(void)  { s_pending = EV_ERR; dispatch(); }
