#ifndef __ADBMS_TIMING_H__
#define __ADBMS_TIMING_H__

/*
 * ADBMS1818 / LTC6813 — Timing Calculator
 * =======================================
 * Datasheet-grounded propagation, wake and ADC-conversion timing.
 * Pure C, no HAL — usable on any platform.
 *
 * Key datasheet facts these helpers encode:
 *
 *  • Daisy-chain ADC conversions start SIMULTANEOUSLY on every device, so the
 *    wait after an ADCV/ADAX/ADSTAT does NOT scale with device count — you
 *    wait only the raw conversion time (Table 4 / 21 / 23 / 25).
 *
 *  • Device count DOES matter for two things:
 *      – Wake-up:       N × tWAKE (from sleep) or N × tREADY (from standby)
 *      – Inter-command: CSB-high gap t5/t6 grows by 70 ns per device
 *
 *  • isoSPI: max 1 Mbps, tCLK ≥ 1 µs. For cables > 10 m extend the clock:
 *      tCLK > 0.9 µs + 2 × tCABLE   (tCABLE ≈ 5 ns per metre)
 */

#include <stdint.h>
#include "dataSetList.h"   /* MD enum */

#ifdef __cplusplus
extern "C" {
#endif

/* ── Per-device propagation (worst-case, ns) ──────────────────────────────── */
#define ADBMS_TDSY_DATA_NS   300u   /* data propagation per device (tDSY(D))   */
#define ADBMS_TDSY_CS_NS     180u   /* CSB propagation per device (tDSY(CS))   */
#define ADBMS_TLAG_NS         70u   /* data-vs-CSB lag per device (tLAG)       */

/* ── Wake / reference timing (worst-case, µs) ─────────────────────────────── */
#define ADBMS_TWAKE_US       400u   /* sleep → ready, per device (tWAKE)       */
#define ADBMS_TREADY_US       10u   /* standby → ready, per device (tREADY)    */
#define ADBMS_TREFUP_US     4400u   /* reference power-up (tREFUP)             */
#define ADBMS_TIDLE_MIN_US  4300u   /* isoSPI idle timeout, minimum (tIDLE)    */

/* ── isoSPI limits ────────────────────────────────────────────────────────── */
#define ADBMS_TCLK_MIN_NS   1000u   /* min SPI clock period (1 MHz)            */
#define ADBMS_CABLE_NS_PER_M   5u   /* cable propagation (~0.2 m/ns)           */

typedef enum {
    ADBMS_FROM_SLEEP   = 0,   /* device fully asleep   → use tWAKE  */
    ADBMS_FROM_STANDBY = 1    /* device in standby     → use tREADY */
} adbms_wake_src_t;

/* ── Power-up / wake ──────────────────────────────────────────────────────── */

/* Total time to wake an N-device chain (µs).
 * from_sleep  → N × tWAKE  (400 µs each)
 * from_standby→ N × tREADY (10 µs each) */
uint32_t adbms_wakeTime_us(uint8_t n_ics, adbms_wake_src_t src);

/* ── Inter-command CSB-high gap (t6, worst-case ns) ───────────────────────────
 * Minimum time CSB must stay HIGH between two daisy-chain commands.
 * t6 > (N × 70 ns) + 950 ns. */
uint32_t adbms_cmdGap_ns(uint8_t n_ics);

/* ── End-to-end data propagation across the chain (ns) ─────────────────────── */
uint32_t adbms_propDelay_ns(uint8_t n_ics);

/* ── Minimum tCLK for a given cable length (ns) ───────────────────────────────
 * Returns max(1000, 900 + 2 × len_m × 5). Use to clamp SPI baud for long cables. */
uint32_t adbms_minTclk_ns(uint32_t cable_len_m);

/* ── ADC conversion times (µs) ────────────────────────────────────────────────
 * md     : MD enum (ADC_422_HZ / ADC_27_KHZ / ADC_7_KHZ / ADC_26_HZ)
 * adcopt : ADCOPT bit (0 = standard mode set, 1 = alternate mode set)
 *
 * With adcopt=0 the four MD codes map to 422 Hz / 27 kHz / 7 kHz / 26 Hz.
 * With adcopt=1 they map to   1 kHz / 14 kHz /  3 kHz /  2 kHz.
 * Values are datasheet maximums where guaranteed, typicals otherwise. */
uint32_t adbms_adcvTime_us  (MD md, uint8_t adcopt);   /* ADCV   — all 18 cells */
uint32_t adbms_adaxTime_us  (MD md, uint8_t adcopt);   /* ADAX   — all GPIO+ref */
uint32_t adbms_adstatTime_us(MD md, uint8_t adcopt);   /* ADSTAT — SC/ITMP/VA/VD */

/* ── Total fixed wait to replace PLADC polling (µs) ───────────────────────────
 * conv_us      : conversion time from one of the helpers above
 * from_standby : 1 → add one-shot tREFUP (reference was off), 0 → reference live
 * Device count is intentionally NOT a parameter (simultaneous conversion). */
uint32_t adbms_pollWait_us(uint32_t conv_us, uint8_t from_standby);

#ifdef __cplusplus
}
#endif

#endif /* __ADBMS_TIMING_H__ */
