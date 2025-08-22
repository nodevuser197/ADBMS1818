#include "adbmsTiming.h"

/* ── Conversion-time lookup tables (µs) ───────────────────────────────────────
 * Indexed by [adcopt][MD].  MD order matches the enum: 0,1,2,3.
 *   adcopt=0 → { 422 Hz, 27 kHz,  7 kHz,   26 Hz }
 *   adcopt=1 → {   1 kHz, 14 kHz,  3 kHz,   2 kHz }
 * Source: ADBMS1818 datasheet Tables 4 / 21 / 23 / 25 (max where guaranteed). */

static const uint32_t ADCV_US[2][4] = {
    /* adcopt=0 */ { 12816u,   1191u,   2488u, 213800u },
    /* adcopt=1 */ {  7230u,   1296u,   3041u,   4437u },
};

static const uint32_t ADAX_US[2][4] = {
    /* adcopt=0 */ { 21316u,   1825u,   3862u, 335498u },
    /* adcopt=1 */ { 12007u,   2116u,   5025u,   7353u },
};

static const uint32_t ADSTAT_US[2][4] = {
    /* adcopt=0 */ {  8538u,    742u,   1556u, 134211u },
    /* adcopt=1 */ {  4814u,    858u,   2022u,   2953u },
};

/* ── Power-up / wake ──────────────────────────────────────────────────────── */

uint32_t adbms_wakeTime_us(uint8_t n_ics, adbms_wake_src_t src)
{
    uint32_t per = (src == ADBMS_FROM_SLEEP) ? ADBMS_TWAKE_US : ADBMS_TREADY_US;
    return (uint32_t)n_ics * per;
}

/* ── Inter-command CSB-high gap (t6) ──────────────────────────────────────── */

uint32_t adbms_cmdGap_ns(uint8_t n_ics)
{
    return ((uint32_t)n_ics * ADBMS_TLAG_NS) + 950u;
}

/* ── End-to-end chain propagation ─────────────────────────────────────────── */

uint32_t adbms_propDelay_ns(uint8_t n_ics)
{
    return (uint32_t)n_ics * ADBMS_TDSY_DATA_NS;
}

/* ── Minimum tCLK for a cable length ──────────────────────────────────────── */

uint32_t adbms_minTclk_ns(uint32_t cable_len_m)
{
    uint32_t need = 900u + (2u * cable_len_m * ADBMS_CABLE_NS_PER_M);
    return (need < ADBMS_TCLK_MIN_NS) ? ADBMS_TCLK_MIN_NS : need;
}

/* ── ADC conversion-time lookups ──────────────────────────────────────────── */

static uint32_t lookup(const uint32_t table[2][4], MD md, uint8_t adcopt)
{
    uint8_t a = adcopt ? 1u : 0u;
    uint8_t m = (uint8_t)md & 0x03u;
    return table[a][m];
}

uint32_t adbms_adcvTime_us(MD md, uint8_t adcopt)   { return lookup(ADCV_US,   md, adcopt); }
uint32_t adbms_adaxTime_us(MD md, uint8_t adcopt)   { return lookup(ADAX_US,   md, adcopt); }
uint32_t adbms_adstatTime_us(MD md, uint8_t adcopt) { return lookup(ADSTAT_US, md, adcopt); }

/* ── Fixed wait replacing PLADC polling ───────────────────────────────────── */

uint32_t adbms_pollWait_us(uint32_t conv_us, uint8_t from_standby)
{
    return conv_us + (from_standby ? ADBMS_TREFUP_US : 0u);
}
