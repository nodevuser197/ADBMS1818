#ifndef __COMM_HELPER_H__
#define __COMM_HELPER_H__

#include "dataSetList.h"

/*
 * COMM Register — Generic I2C / SPI Frame Builder
 * ================================================
 * The ADBMS1818 COMM register (6 bytes = 3 slots) lets the IC act as an
 * I2C or SPI master over its GPIO pins.  One "frame" covers 3 byte slots.
 * Transactions longer than 3 bytes require chained frames.
 *
 * Each slot layout (2 raw bytes in COMM register):
 *   Byte 2n  : ICOMn[3:0] | Dn[7:4]
 *   Byte 2n+1: Dn[3:0]   | FCOMn[3:0]
 *
 * ICOMn[3] = 0 → I2C master mode
 * ICOMn[3] = 1 → SPI master mode
 *
 * GPIO mapping
 *   I2C : SDA = GPIO1,  SCL = GPIO2
 *   SPI : SDI = GPIO1,  SDO = GPIO2,  SCK = GPIO3,  CSBM = GPIO4
 *
 * ── Full transaction sequence ──────────────────────────────────────────────
 *
 *   1. Fill comm_ with one of the helpers below
 *   2. createComm(n, ic)          — pack struct → ic_register.tx_data
 *   3. adbms1818_Wrcomm(n, ic)    — WRCOMM command over isoSPI
 *   4. adbms1818_Stcomm()         — STCOMM: execute I2C/SPI frame
 *   5. adbms1818_Rdcomm(n, ic)    — RDCOMM: read back received bytes  (RX only)
 *   6. parseComm(n, ic, rx_data)  — unpack ic_register.rx_data → comm.data[]
 *
 * Repeat steps 1-4 (or 1-6) for each additional frame.
 * ──────────────────────────────────────────────────────────────────────────
 *
 * ── I2C frame capacity ─────────────────────────────────────────────────────
 *
 *   Frame-start (comm_i2c_write / comm_i2c_read):
 *     Slot 0 = START + address byte  →  max 2 data bytes per frame
 *
 *   Continuation (comm_i2c_write_cont / comm_i2c_read_cont):
 *     No address slot               →  max 3 data bytes per frame
 *
 *   stop = 1 → last slot gets NACK + STOP (ends the I2C transaction)
 *   stop = 0 → last slot gets master ACK  (more frames follow)
 *
 * ── SPI frame capacity ─────────────────────────────────────────────────────
 *
 *   Frame-start (comm_spi_write / comm_spi_read):
 *     Slot 0 asserts CS_LOW         →  max 3 data bytes per frame
 *
 *   Continuation (comm_spi_write_cont / comm_spi_read_cont):
 *     CS already low                →  max 3 data bytes per frame
 *
 *   deassert = 1 → last slot gets CS_HIGH (ends the SPI transaction)
 *   deassert = 0 → CS stays low   (more frames follow)
 *
 * ── Example: I2C — write 3 bytes to register 0x10 of device at 0x50 ───────
 *
 *   uint8_t payload[2] = {0x10, 0xAB};   // reg addr, value
 *
 *   // Frame 1: START + dev_addr + reg_addr + value_byte → STOP
 *   comm_i2c_write(&ic->comm, 0x50, payload, 2, 1);
 *   createComm(1, ic);
 *   adbms1818_Wrcomm(1, ic);
 *   adbms1818_Stcomm();
 *
 * ── Example: I2C — read 4 bytes from device at 0x50 ───────────────────────
 *
 *   // Frame 1: START + addr+R → read 2 bytes (keep bus open)
 *   comm_i2c_read(&ic->comm, 0x50, 2, 0);
 *   createComm(1, ic);  adbms1818_Wrcomm(1, ic);  adbms1818_Stcomm();
 *   adbms1818_Rdcomm(1, ic);
 *   parseComm(1, ic, ic->ic_register.rx_data);
 *   // ic->comm.data[1], [2] hold the received bytes
 *
 *   // Frame 2: 2 more bytes → STOP
 *   comm_i2c_read_cont(&ic->comm, 2, 1);
 *   createComm(1, ic);  adbms1818_Wrcomm(1, ic);  adbms1818_Stcomm();
 *   adbms1818_Rdcomm(1, ic);
 *   parseComm(1, ic, ic->ic_register.rx_data);
 *
 * ── Example: SPI — send command byte 0x9C, read 2 bytes back ───────────────
 *
 *   uint8_t cmd = 0x9C;
 *
 *   // Frame 1: CS↓ + cmd + 2 dummy RX bytes + CS↑
 *   comm_spi_write(&ic->comm, &cmd, 1, 0);          // assert CS, send cmd
 *   ic->comm.icomm[1] = IBLANK;                     // continue frame
 *   ic->comm.data[1]  = 0xFF;                       // dummy TX
 *   ic->comm.fcomm[1] = FSPI_CS_HOLD;
 *   ic->comm.icomm[2] = IBLANK;
 *   ic->comm.data[2]  = 0xFF;
 *   ic->comm.fcomm[2] = FSPI_CS_HIGH;               // deassert after last byte
 *   createComm(1, ic);  adbms1818_Wrcomm(1, ic);  adbms1818_Stcomm();
 *   adbms1818_Rdcomm(1, ic);
 *   parseComm(1, ic, ic->ic_register.rx_data);
 *   // ic->comm.data[1], [2] hold the 2 received bytes
 *
 * ── Adding a new external IC ───────────────────────────────────────────────
 *
 *   1. Define the IC's address and register map at the top of your file.
 *   2. Write a read and/or write helper in application.c that calls these
 *      frame builders and chains as many frames as needed.
 *   3. Call your helper from the main loop or task scheduler.
 *   No changes to genericType, parseCreate, or the lower layers are needed.
 * ──────────────────────────────────────────────────────────────────────────
 */

/* ── I2C helpers ─────────────────────────────────────────────────────────── */

/* Frame-start write: slot 0 = START + (addr7<<1|W), slots 1-2 = data[].
 * len : number of data bytes to send (0-2).
 * stop: 1 → NACK+STOP on last byte, 0 → leave bus open for next frame. */
void comm_i2c_write(comm_ *c, uint8_t addr7, const uint8_t *data, uint8_t len, uint8_t stop);

/* Continuation write: no START, 3 data slots.
 * len : 1-3. stop: same as above. */
void comm_i2c_write_cont(comm_ *c, const uint8_t *data, uint8_t len, uint8_t stop);

/* Frame-start read: slot 0 = START + (addr7<<1|R), slots 1-2 = dummy RX.
 * len : number of bytes to read (0-2).
 * stop: 1 → master NACK + STOP on last byte, 0 → master ACK (more frames). */
void comm_i2c_read(comm_ *c, uint8_t addr7, uint8_t len, uint8_t stop);

/* Continuation read: no START, 3 dummy RX slots.
 * len : 1-3. stop: same as above. */
void comm_i2c_read_cont(comm_ *c, uint8_t len, uint8_t stop);

/* ── SPI helpers ─────────────────────────────────────────────────────────── */

/* Frame-start write: slot 0 asserts CS_LOW, all slots carry data[].
 * len      : 1-3.
 * deassert : 1 → CS_HIGH after last byte, 0 → CS stays low. */
void comm_spi_write(comm_ *c, const uint8_t *data, uint8_t len, uint8_t deassert);

/* Continuation write: CS already low, 3 data slots. */
void comm_spi_write_cont(comm_ *c, const uint8_t *data, uint8_t len, uint8_t deassert);

/* Frame-start read: slot 0 asserts CS_LOW, all slots TX 0xFF (dummy).
 * Use adbms1818_Rdcomm + parseComm after STCOMM to retrieve RX bytes. */
void comm_spi_read(comm_ *c, uint8_t len, uint8_t deassert);

/* Continuation read: CS already low, 3 dummy TX slots. */
void comm_spi_read_cont(comm_ *c, uint8_t len, uint8_t deassert);

#endif
