#include "genericType.h"
#include "commandList.h"
#include "parseCreate.h"

/* Precomputed CRC15 Table */
const uint16_t Crc15Table[256] = {
    0x0000, 0xc599, 0xceab, 0xb32, 0xd8cf, 0x1d56, 0x1664, 0xd3fd,
    0xf407, 0x319e, 0x3aac, 0xff35, 0x2cc8, 0xe951, 0xe263, 0x27fa,
    0xad97, 0x680e, 0x633c, 0xa6a5, 0x7558, 0xb0c1, 0xbbf3, 0x7e6a,
    0x5990, 0x9c09, 0x973b, 0x52a2, 0x815f, 0x44c6, 0x4ff4, 0x8a6d,
    0x5b2e, 0x9eb7, 0x9585, 0x501c, 0x83e1, 0x4678, 0x4d4a, 0x88d3,
    0xaf29, 0x6ab0, 0x6182, 0xa41b, 0x77e6, 0xb27f, 0xb94d, 0x7cd4,
    0xf6b9, 0x3320, 0x3812, 0xfd8b, 0x2e76, 0xebef, 0xe0dd, 0x2544,
    0x2be, 0xc727, 0xcc15, 0x98c, 0xda71, 0x1fe8, 0x14da, 0xd143,
    0xf3c5, 0x365c, 0x3d6e, 0xf8f7, 0x2b0a, 0xee93, 0xe5a1, 0x2038,
    0x7c2, 0xc25b, 0xc969, 0xcf0, 0xdf0d, 0x1a94, 0x11a6, 0xd43f,
    0x5e52, 0x9bcb, 0x90f9, 0x5560, 0x869d, 0x4304, 0x4836, 0x8daf,
    0xaa55, 0x6fcc, 0x64fe, 0xa167, 0x729a, 0xb703, 0xbc31, 0x79a8,
    0xa8eb, 0x6d72, 0x6640, 0xa3d9, 0x7024, 0xb5bd, 0xbe8f, 0x7b16,
    0x5cec, 0x9975, 0x9247, 0x57de, 0x8423, 0x41ba, 0x4a88, 0x8f11,
    0x57c, 0xc0e5, 0xcbd7, 0xe4e, 0xddb3, 0x182a, 0x1318, 0xd681,
    0xf17b, 0x34e2, 0x3fd0, 0xfa49, 0x29b4, 0xec2d, 0xe71f, 0x2286,
    0xa213, 0x678a, 0x6cb8, 0xa921, 0x7adc, 0xbf45, 0xb477, 0x71ee,
    0x5614, 0x938d, 0x98bf, 0x5d26, 0x8edb, 0x4b42, 0x4070, 0x85e9,
    0xf84, 0xca1d, 0xc12f, 0x4b6, 0xd74b, 0x12d2, 0x19e0, 0xdc79,
    0xfb83, 0x3e1a, 0x3528, 0xf0b1, 0x234c, 0xe6d5, 0xede7, 0x287e,
    0xf93d, 0x3ca4, 0x3796, 0xf20f, 0x21f2, 0xe46b, 0xef59, 0x2ac0,
    0xd3a, 0xc8a3, 0xc391, 0x608, 0xd5f5, 0x106c, 0x1b5e, 0xdec7,
    0x54aa, 0x9133, 0x9a01, 0x5f98, 0x8c65, 0x49fc, 0x42ce, 0x8757,
    0xa0ad, 0x6534, 0x6e06, 0xab9f, 0x7862, 0xbdfb, 0xb6c9, 0x7350,
    0x51d6, 0x944f, 0x9f7d, 0x5ae4, 0x8919, 0x4c80, 0x47b2, 0x822b,
    0xa5d1, 0x6048, 0x6b7a, 0xaee3, 0x7d1e, 0xb887, 0xb3b5, 0x762c,
    0xfc41, 0x39d8, 0x32ea, 0xf773, 0x248e, 0xe117, 0xea25, 0x2fbc,
    0x846, 0xcddf, 0xc6ed, 0x374, 0xd089, 0x1510, 0x1e22, 0xdbbb,
    0xaf8, 0xcf61, 0xc453, 0x1ca, 0xd237, 0x17ae, 0x1c9c, 0xd905,
    0xfeff, 0x3b66, 0x3054, 0xf5cd, 0x2630, 0xe3a9, 0xe89b, 0x2d02,
    0xa76f, 0x62f6, 0x69c4, 0xac5d, 0x7fa0, 0xba39, 0xb10b, 0x7492,
    0x5368, 0x96f1, 0x9dc3, 0x585a, 0x8ba7, 0x4e3e, 0x450c, 0x8095
};

uint16_t Pec15_Calc(uint8_t len, uint8_t *data)
{
    uint16_t remainder, addr;
    remainder = 16;
    for (uint8_t i = 0; i < len; i++) {
        addr = ((remainder >> 7) ^ data[i]) & 0xff;
        remainder = (remainder << 8) ^ Crc15Table[addr];
    }
    return (remainder * 2);
}

void spiSendCmd(uint8_t tx_cmd[2])
{
    uint8_t cmd[4];
    uint16_t cmd_pec;

    cmd[0] = tx_cmd[0];
    cmd[1] = tx_cmd[1];
    cmd_pec = Pec15_Calc(2, cmd);
    cmd[2] = (uint8_t)(cmd_pec >> 8);
    cmd[3] = (uint8_t)(cmd_pec);
    ltcCSLow();
    spiWriteBytes(cmd, 4);
    ltcCSHigh();
}

/* Like spiSendCmd but leaves CS LOW — caller must call ltcCSHigh() when done. */
void spiSendCmdKeepCS(uint8_t tx_cmd[2])
{
    uint8_t cmd[4];
    uint16_t cmd_pec;

    cmd[0] = tx_cmd[0];
    cmd[1] = tx_cmd[1];
    cmd_pec = Pec15_Calc(2, cmd);
    cmd[2] = (uint8_t)(cmd_pec >> 8);
    cmd[3] = (uint8_t)(cmd_pec);
    ltcCSLow();
    spiWriteBytes(cmd, 4);
}

void spiReadData(uint8_t tIC, uint8_t tx_cmd[2], uint8_t *rx_data,
                 uint8_t *pec_error, uint8_t regLength)
{
    uint8_t *data, *copyArray;
    uint16_t cmd_pec, received_pec, calc_pec;
    uint8_t BYTES_IN_REG = regLength;
    uint8_t DATA_SIZE = regLength - 2;
    uint8_t RX_BUFFER = (regLength * tIC);

    data = (uint8_t *)calloc(RX_BUFFER, sizeof(uint8_t));
    copyArray = (uint8_t *)calloc(DATA_SIZE, sizeof(uint8_t));

    if ((data == NULL) || (copyArray == NULL)) {
        // printf("Failed to allocate memory\n");
        if (data) free(data);
        if (copyArray) free(copyArray);
        return;
    }

    uint8_t cmd[4];
    cmd[0] = tx_cmd[0];
    cmd[1] = tx_cmd[1];
    cmd_pec = Pec15_Calc(2, cmd);
    cmd[2] = (uint8_t)(cmd_pec >> 8);
    cmd[3] = (uint8_t)(cmd_pec);

//    ltcWakeUp(tIC);
    ltcCSLow();
    spiWriteReadBytes(cmd, 4, data, RX_BUFFER);
    ltcCSHigh();

    for (uint8_t current_ic = 0; current_ic < tIC; current_ic++) {
        // Copy data bytes (excluding PEC)
        for (uint8_t current_byte = 0; current_byte < DATA_SIZE; current_byte++) {
            rx_data[(current_ic * DATA_SIZE) + current_byte] =
                data[current_byte + (current_ic * BYTES_IN_REG)];
        }

        // Extract PEC
        received_pec = (uint16_t)(data[(current_ic * BYTES_IN_REG) + DATA_SIZE] << 8) |
                       data[(current_ic * BYTES_IN_REG) + DATA_SIZE + 1];

        // Copy data for PEC calculation
        for (uint8_t i = 0; i < DATA_SIZE; i++) {
            copyArray[i] = data[(current_ic * BYTES_IN_REG) + i];
        }

        // Calculate PEC
        calc_pec = Pec15_Calc(DATA_SIZE, copyArray);

        // Set error flag
        pec_error[current_ic] = (received_pec != calc_pec) ? 1 : 0;
    }

    free(data);
    free(copyArray);
}

void spiWriteData(uint8_t tIC, uint8_t tx_cmd[2], uint8_t *data)
{
    uint8_t BYTES_IN_REG = TX_DATA;
    uint8_t CMD_LEN = 4 + ((TX_DATA + 2) * tIC); // +2 for PEC per IC //RX DATA OLABILIR
    uint16_t data_pec, cmd_pec;
    uint8_t *cmd, copyArray[TX_DATA];
    uint8_t cmd_index;

    cmd = (uint8_t *)calloc(CMD_LEN, sizeof(uint8_t));
    if (cmd == NULL) {
        printf("Failed to allocate cmd array memory\n");
        return;
        while(1);
    }

    // Command header
    cmd[0] = tx_cmd[0];
    cmd[1] = tx_cmd[1];
    cmd_pec = Pec15_Calc(2, cmd);
    cmd[2] = (uint8_t)(cmd_pec >> 8);
    cmd[3] = (uint8_t)(cmd_pec);
    cmd_index = 4;

    for (uint8_t current_ic = tIC; current_ic > 0; current_ic--) {
        uint8_t src_address = (current_ic - 1) * TX_DATA;

        for (uint8_t current_byte = 0; current_byte < BYTES_IN_REG; current_byte++) {
            cmd[cmd_index] = data[src_address + current_byte];
            copyArray[current_byte] = data[src_address + current_byte];
            cmd_index++;
        }

        // PEC
        data_pec = Pec15_Calc(BYTES_IN_REG, copyArray);
        cmd[cmd_index++] = (uint8_t)(data_pec >> 8);
        cmd[cmd_index++] = (uint8_t)data_pec;
    }
    ltcWakeUp(tIC);
    ltcCSLow();
    spiWriteBytes(cmd, CMD_LEN);
    ltcCSHigh();

    free(cmd);
}

void adbms1818ReadData(uint8_t tIC, cell_asic_ *ic, uint8_t cmd_arg[2],
                       TYPE type, GRP group)
{
    uint8_t regData_size = 8; // Her zaman 8 byte (6 data + 2 PEC)
    uint16_t rBuff_size = tIC * regData_size; // Single group için

    uint8_t *read_buffer = (uint8_t *)calloc(rBuff_size, sizeof(uint8_t));
    uint8_t *pec_error = (uint8_t *)calloc(tIC, sizeof(uint8_t));

    if (!read_buffer || !pec_error) {
        printf("Failed to allocate memory\n");
        if (read_buffer) free(read_buffer);
        if (pec_error) free(pec_error);
        return;
    }

    switch (type) {
        case Config:
            spiReadData(tIC, cmd_arg, read_buffer, pec_error, regData_size);
            if (group == A) {
                parseConfigA(tIC, ic, group, read_buffer);
            } else if (group == B) {
                parseConfigB(tIC, ic, group, read_buffer);
            }
            for (uint8_t cic = 0; cic < tIC; cic++) {
                ic[cic].cccrc.cfgr_pec = pec_error[cic];
            }
            break;

        case Cell:
            spiReadData(tIC, cmd_arg, read_buffer, pec_error, regData_size);
            parseCellVoltage(tIC, ic, group, read_buffer);
            for (uint8_t cic = 0; cic < tIC; cic++) {
                ic[cic].cccrc.cell_pec = pec_error[cic];
            }
            break;

        case Rdcvall:
            // ALL_GRP için application layer'da 6 ayrı komut gönderilmeli
            // Bu function tek grup okuyor
            printf("Rdcvall should be handled by multiple Cell reads\n");
            break;

        case Aux:
            spiReadData(tIC, cmd_arg, read_buffer, pec_error, regData_size);
            parseAuxVoltage(tIC, ic, group, read_buffer);
            for (uint8_t cic = 0; cic < tIC; cic++) {
                ic[cic].cccrc.aux_pec = pec_error[cic];
            }
            break;

        case Rdauxall:
            printf("Rdauxall should be handled by multiple Aux reads\n");
            break;

        case Status:
            spiReadData(tIC, cmd_arg, read_buffer, pec_error, regData_size);
            parseStatus(tIC, ic, group, read_buffer);
            for (uint8_t cic = 0; cic < tIC; cic++) {
                ic[cic].cccrc.stat_pec = pec_error[cic];
            }
            break;

        case Rdstatall:
            printf("Rdstatall should be handled by multiple Status reads\n");
            break;

        case Comm:
            spiReadData(tIC, cmd_arg, read_buffer, pec_error, regData_size);
            parseComm(tIC, ic, read_buffer);
            for (uint8_t cic = 0; cic < tIC; cic++) {
                ic[cic].cccrc.comm_pec = pec_error[cic];
            }
            break;

        default:
            break;
    }

    free(read_buffer);
    free(pec_error);
}
void adbms1818WriteData(uint8_t tIC, cell_asic_ *ic, uint8_t cmd_arg[2],
                        TYPE type, GRP group)
{
    uint8_t data_len = TX_DATA, write_size = (TX_DATA * tIC);
    uint8_t *write_buffer = (uint8_t *)calloc(write_size, sizeof(uint8_t));

    if (write_buffer == NULL) {
        printf("Failed to allocate write_buffer memory\n");
        return;
    }

    switch (type) {
        case Config:
            switch (group) {
                case A:
                    createConfigA(tIC, ic);
                    for (uint8_t cic = 0; cic < tIC; cic++) {
                        for (uint8_t data = 0; data < data_len; data++) {
                            write_buffer[(cic * data_len) + data] = ic[cic].ic_register.tx_data[data];
                        }
                    }
                    break;
                case B:
                    createConfigB(tIC, ic);
                    for (uint8_t cic = 0; cic < tIC; cic++) {
                        for (uint8_t data = 0; data < data_len; data++) {
                            write_buffer[(cic * data_len) + data] = ic[cic].ic_register.tx_data[data];
                        }
                    }
                    break;
                default:
                    break;
            }
            break;

        case Comm:
            createComm(tIC, ic);
            for (uint8_t cic = 0; cic < tIC; cic++) {
                for (uint8_t data = 0; data < data_len; data++) {
                    write_buffer[(cic * data_len) + data] = ic[cic].ic_register.tx_data[data];
                }
            }
            break;

        default:
            break;
    }

    spiWriteData(tIC, cmd_arg, write_buffer);
    free(write_buffer);
}

// ADC Commands
void adbms1818_Adcv(MD md, DCP dcp, CH ch)
{
    uint8_t cmd[2];
    cmd[0] = 0x02 | ((md & 0x02) >> 1);
    cmd[1] = ((md & 0x01) << 7) | ((dcp & 0x01) << 4) | (ch & 0x07) | 0x60;
    spiSendCmd(cmd);
}

void adbms1818_Adcvax(MD md, DCP dcp)
{
    uint8_t cmd[2];
    cmd[0] = 0x02 | ((md & 0x02) >> 1);
    cmd[1] = ((md & 0x01) << 7) | ((dcp & 0x01) << 4) | 0x6F;
    spiSendCmd(cmd);
}

void adbms1818_Adcvsc(MD md, DCP dcp)
{
    uint8_t cmd[2];
    cmd[0] = 0x06 | ((md & 0x02) >> 1);
    cmd[1] = ((md & 0x01) << 7) | ((dcp & 0x01) << 4) | 0x67;
    spiSendCmd(cmd);
}

void adbms1818_Adax(MD md, CHG chg)
{
    uint8_t cmd[2];
    cmd[0] = 0x04 | ((md & 0x02) >> 1);
    cmd[1] = ((md & 0x01) << 7) | (chg & 0x07) | 0x60;
    spiSendCmd(cmd);
}

void adbms1818_Adstat(MD md, CHST chst)
{
    uint8_t cmd[2];
    cmd[0] = 0x04 | ((md & 0x02) >> 1);
    cmd[1] = ((md & 0x01) << 7) | (chst & 0x07) | 0x68;
    spiSendCmd(cmd);
}

void adbms1818_Adol(MD md, DCP dcp)
{
    uint8_t cmd[2];
    cmd[0] = 0x02 | ((md & 0x02) >> 1);
    cmd[1] = ((md & 0x01) << 7) | ((dcp & 0x01) << 4) | 0x01;
    spiSendCmd(cmd);
}

// Open wire detection
void adbms1818_Adow(MD md, PUP pup, CH ch)
{
    uint8_t cmd[2];
    cmd[0] = 0x02 | ((md & 0x02) >> 1);
    cmd[1] = ((md & 0x01) << 7) | ((pup & 0x01) << 6) | (ch & 0x07) | 0x28;
    spiSendCmd(cmd);
}

void adbms1818_Axow(MD md, PUP pup, CHG chg)
{
    uint8_t cmd[2];
    cmd[0] = 0x04 | ((md & 0x02) >> 1);
    cmd[1] = ((md & 0x01) << 7) | ((pup & 0x01) << 6) | (chg & 0x07) | 0x10;
    spiSendCmd(cmd);
}

// Self test commands
void adbms1818_Cvst(MD md, ST st)
{
    uint8_t cmd[2];
    cmd[0] = 0x02 | ((md & 0x02) >> 1);
    cmd[1] = ((md & 0x01) << 7) | ((st & 0x03) << 5) | 0x07;
    spiSendCmd(cmd);
}

void adbms1818_Axst(MD md, ST st)
{
    uint8_t cmd[2];
    cmd[0] = 0x04 | ((md & 0x02) >> 1);
    cmd[1] = ((md & 0x01) << 7) | ((st & 0x03) << 5) | 0x07;
    spiSendCmd(cmd);
}

void adbms1818_Statst(MD md, ST st)
{
    uint8_t cmd[2];
    cmd[0] = 0x04 | ((md & 0x02) >> 1);
    cmd[1] = ((md & 0x01) << 7) | ((st & 0x03) << 5) | 0x15;
    spiSendCmd(cmd);
}

// Clear commands
void adbms1818_Clrcell(void)
{
    uint8_t cmd[2] = {0x07, 0x11};
    spiSendCmd(cmd);
}

void adbms1818_Clraux(void)
{
    uint8_t cmd[2] = {0x07, 0x12};
    spiSendCmd(cmd);
}

void adbms1818_Clrstat(void)
{
    uint8_t cmd[2] = {0x07, 0x13};
    spiSendCmd(cmd);
}

// Diagnostic commands
void adbms1818_Diagn(void)
{
    uint8_t cmd[2] = {0x07, 0x15};
    spiSendCmd(cmd);
}

void adbms1818_Pladc(void)
{
    uint8_t cmd[2] = {0x07, 0x14};
    spiSendCmd(cmd);
}

// Mute/Unmute commands
void adbms1818_Mute(void)
{
    uint8_t cmd[2] = {0x00, 0x28};
    spiSendCmd(cmd);
}

void adbms1818_Unmute(void)
{
    uint8_t cmd[2] = {0x00, 0x29};
    spiSendCmd(cmd);
}

// Communication commands
void adbms1818_Wrcomm(uint8_t tIC, cell_asic_ *ic)
{
    uint8_t cmd[2] = {0x07, 0x21};
    adbms1818WriteData(tIC, ic, cmd, Comm, A);
}

void adbms1818_Rdcomm(uint8_t tIC, cell_asic_ *ic)
{
    uint8_t cmd[2] = {0x07, 0x22};
    adbms1818ReadData(tIC, ic, cmd, Comm, A);
}

void adbms1818_Stcomm(void)
{
    /* CS stays LOW through command AND 72 dummy clocks (9 bytes × 8 bits).
       MOSI is don't care during execution — ADBMS drives I2C/SPI from COMM register. */
    uint8_t cmd[2] = {0x07, 0x23};
    uint8_t dummy[9] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

    spiSendCmdKeepCS(cmd);
    spiWriteBytes(dummy, 9);
    ltcCSHigh();
}

uint32_t adbms1818PollAdc(uint8_t tx_cmd[2])
{
    uint8_t sdo_state;

    spiSendCmd(tx_cmd);

    ltcCSLow();
    do {
        spiReadBytes(&sdo_state, 1);
    } while(sdo_state != 0xFF);
    ltcCSHigh();

    return 0; // Timer yok, sadece completion check
}
