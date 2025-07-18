#include "commHelper.h"
#include <string.h>

/* ── internal helpers ────────────────────────────────────────────────────── */

static void slot_set(comm_ *c, uint8_t i, uint8_t icomm, uint8_t data, uint8_t fcomm)
{
    c->icomm[i] = icomm;
    c->data[i]  = data;
    c->fcomm[i] = fcomm;
}

static void slots_fill_i2c_nop(comm_ *c, uint8_t from)
{
    for (uint8_t i = from; i < 3; i++)
        slot_set(c, i, INOTRANS, 0x00, FMASTER_ACK);
}

static void slots_fill_spi_nop(comm_ *c, uint8_t from)
{
    for (uint8_t i = from; i < 3; i++)
        slot_set(c, i, ISPI_NOTRANS, 0x00, FSPI_CS_HOLD);
}

/* ── I2C write ───────────────────────────────────────────────────────────── */

void comm_i2c_write(comm_ *c, uint8_t addr7, const uint8_t *data, uint8_t len, uint8_t stop)
{
    memset(c, 0, sizeof(comm_));
    slot_set(c, 0, ISTART, (uint8_t)((addr7 << 1) | 0x00), FMASTER_ACK);
    uint8_t n = len > 2 ? 2 : len;
    for (uint8_t i = 0; i < n; i++) {
        uint8_t fcomm = (stop && i == n - 1) ? FMASTER_NACK_STOP : FMASTER_ACK;
        slot_set(c, i + 1, IBLANK, data[i], fcomm);
    }
    slots_fill_i2c_nop(c, 1 + n);
}

void comm_i2c_write_cont(comm_ *c, const uint8_t *data, uint8_t len, uint8_t stop)
{
    memset(c, 0, sizeof(comm_));
    uint8_t n = len > 3 ? 3 : len;
    for (uint8_t i = 0; i < n; i++) {
        uint8_t fcomm = (stop && i == n - 1) ? FMASTER_NACK_STOP : FMASTER_ACK;
        slot_set(c, i, IBLANK, data[i], fcomm);
    }
    slots_fill_i2c_nop(c, n);
}

/* ── I2C read ────────────────────────────────────────────────────────────── */

void comm_i2c_read(comm_ *c, uint8_t addr7, uint8_t len, uint8_t stop)
{
    memset(c, 0, sizeof(comm_));
    slot_set(c, 0, ISTART, (uint8_t)((addr7 << 1) | 0x01), FMASTER_ACK);
    uint8_t n = len > 2 ? 2 : len;
    for (uint8_t i = 0; i < n; i++) {
        /* Master ACKs intermediate bytes; NACK+STOP on the last byte. */
        uint8_t fcomm = (stop && i == n - 1) ? FSLAVE_NACK_STOP : FMASTER_ACK;
        slot_set(c, i + 1, IBLANK, 0xFF, fcomm);
    }
    slots_fill_i2c_nop(c, 1 + n);
}

void comm_i2c_read_cont(comm_ *c, uint8_t len, uint8_t stop)
{
    memset(c, 0, sizeof(comm_));
    uint8_t n = len > 3 ? 3 : len;
    for (uint8_t i = 0; i < n; i++) {
        uint8_t fcomm = (stop && i == n - 1) ? FSLAVE_NACK_STOP : FMASTER_ACK;
        slot_set(c, i, IBLANK, 0xFF, fcomm);
    }
    slots_fill_i2c_nop(c, n);
}

/* ── SPI write ───────────────────────────────────────────────────────────── */

void comm_spi_write(comm_ *c, const uint8_t *data, uint8_t len, uint8_t deassert)
{
    memset(c, 0, sizeof(comm_));
    uint8_t n = len > 3 ? 3 : len;
    for (uint8_t i = 0; i < n; i++) {
        uint8_t icomm = (i == 0) ? ISPI_CS_LOW : IBLANK;
        uint8_t fcomm = (deassert && i == n - 1) ? FSPI_CS_HIGH : FSPI_CS_HOLD;
        slot_set(c, i, icomm, data[i], fcomm);
    }
    slots_fill_spi_nop(c, n);
}

void comm_spi_write_cont(comm_ *c, const uint8_t *data, uint8_t len, uint8_t deassert)
{
    memset(c, 0, sizeof(comm_));
    uint8_t n = len > 3 ? 3 : len;
    for (uint8_t i = 0; i < n; i++) {
        uint8_t fcomm = (deassert && i == n - 1) ? FSPI_CS_HIGH : FSPI_CS_HOLD;
        slot_set(c, i, IBLANK, data[i], fcomm);
    }
    slots_fill_spi_nop(c, n);
}

/* ── SPI read ────────────────────────────────────────────────────────────── */

void comm_spi_read(comm_ *c, uint8_t len, uint8_t deassert)
{
    memset(c, 0, sizeof(comm_));
    uint8_t n = len > 3 ? 3 : len;
    for (uint8_t i = 0; i < n; i++) {
        uint8_t icomm = (i == 0) ? ISPI_CS_LOW : IBLANK;
        uint8_t fcomm = (deassert && i == n - 1) ? FSPI_CS_HIGH : FSPI_CS_HOLD;
        slot_set(c, i, icomm, 0xFF, fcomm);
    }
    slots_fill_spi_nop(c, n);
}

void comm_spi_read_cont(comm_ *c, uint8_t len, uint8_t deassert)
{
    memset(c, 0, sizeof(comm_));
    uint8_t n = len > 3 ? 3 : len;
    for (uint8_t i = 0; i < n; i++) {
        uint8_t fcomm = (deassert && i == n - 1) ? FSPI_CS_HIGH : FSPI_CS_HOLD;
        slot_set(c, i, IBLANK, 0xFF, fcomm);
    }
    slots_fill_spi_nop(c, n);
}
