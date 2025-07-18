#include "parseCreate.h"

uint16_t setOverVoltageLimit(float voltage)
{
    return (uint16_t)(((voltage * 10000) / 16));
}

uint16_t setUnderVoltageLimit(float voltage)
{
    return (uint16_t)(((voltage * 10000) / 16) - 1);
}

uint8_t cfgA_Gpio(uint8_t gpio, COFA_GPIO stat)
{
    uint8_t gpioValue;
    if (stat == GPIO_CLR) {
        gpioValue = (1 << gpio);
    } else {
        gpioValue = (0 << gpio);
    }
    return gpioValue;
}

uint16_t cfgA_DCC(uint16_t dcc, COFA_DCC stat)
{
    uint16_t dccValue;
    if (stat == DCC_ON) {
        dccValue = (1 << dcc);
    } else {
        dccValue = (0 << dcc);
    }
    return dccValue;
}

void setCfgA_DCTO(cell_asic_ *ic, COFA_DCTO value)
{
    ic->tx_cfga.dcto = value;
}

void setCfgA_REFON(cell_asic_ *ic, COFA_REF value)
{
    ic->tx_cfga.refon = value;
}

void setCfgA_DTEN(cell_asic_ *ic, COFA_DTEN value)
{
    ic->tx_cfga.dten = value;
}

void setCfgA_ADCOPT(cell_asic_ *ic, COFA_ADCOPT value)
{
    ic->tx_cfga.adcopt = value;
}

void setCfgA_SelectedGPIO(cell_asic_ *ic, uint16_t gpios, COFA_GPIO stat)
{
    uint8_t mask = 0;
    for (uint8_t gpio = 1; gpio <= 5; gpio++) {
        if (gpios & (1 << gpio)) {
            if (stat == GPIO_CLR)
                mask |= (1 << (gpio + 2));
        }
    }

    if (stat == GPIO_CLR) {
        ic->tx_cfga.gpio |= mask;
    } else {
        ic->tx_cfga.gpio &= ~mask;
    }
}

void setGPIO_FromHex(cell_asic_ *ic, uint8_t gpio, uint8_t hexValue)
{
    if (gpio < 1 || gpio > 5) return;

    COFA_GPIO state = (hexValue == 0) ? GPIO_CLR : GPIO_SET;

    if (state == GPIO_CLR) {
        ic->tx_cfga.gpio |= (1 << (gpio + 2));
    } else {
        ic->tx_cfga.gpio &= ~(1 << (gpio + 2));
    }
}

void setCfgB_GPIO6to9(cell_asic_ *ic, uint8_t gpio_mask)
{
    ic->tx_cfgb.gpio = gpio_mask & 0x0F; // Only use lower 4 bits for GPIO6-9
}

void setCfgB_DCC13to18(cell_asic_ *ic, uint16_t dcc_mask)
{
    ic->tx_cfgb.dcc = dcc_mask;
}

void setCfgB_PS(cell_asic_ *ic, uint8_t ps_value)
{
    ic->tx_cfgb.ps = ps_value & 0x03;
}

void setCfgB_DTMEN(cell_asic_ *ic, uint8_t dtmen)
{
    ic->tx_cfgb.dtmen = dtmen ? 1 : 0;
}
void parseADOL(uint8_t tIC, cell_asic_ *ic, uint8_t *data)
{
    for (uint8_t current_ic = 0; current_ic < tIC; current_ic++) {
        // Cell 7 ADC1 sonucu -> Cell 8 yerine
        ic[current_ic].cell.c_codes[7] = data[current_ic*4] | (data[current_ic*4 + 1] << 8);

        // Cell 13 ADC2 sonucu -> Cell 14 yerine
        ic[current_ic].cell.c_codes[13] = data[current_ic*4 + 2] | (data[current_ic*4 + 3] << 8);
    }
}
void parseConfigA(uint8_t tIC, cell_asic_ *ic, GRP grp, uint8_t *data)
{
    for (uint8_t current_ic = 0; current_ic < tIC; current_ic++) {
        uint8_t *ic_data = &data[current_ic * 6];

        ic[current_ic].rx_cfga.adcopt = ic_data[0] & 0x01;
        ic[current_ic].rx_cfga.dten = (ic_data[0] & 0x02) >> 1;
        ic[current_ic].rx_cfga.refon = (ic_data[0] & 0x04) >> 2;
        ic[current_ic].rx_cfga.gpio = (ic_data[0] & 0xF8) >> 3;

        ic[current_ic].rx_cfga.vuv = ic_data[1] | ((ic_data[2] & 0x0F) << 8);
        ic[current_ic].rx_cfga.vov = ((ic_data[2] & 0xF0) >> 4) | (ic_data[3] << 4);
        ic[current_ic].rx_cfga.dcc = ic_data[4] | ((ic_data[5] & 0x0F) << 8);
        ic[current_ic].rx_cfga.dcto = (ic_data[5] & 0xF0) >> 4;
    }
}

void parseConfigB(uint8_t tIC, cell_asic_ *ic, GRP grp, uint8_t *data)
{
    for (uint8_t current_ic = 0; current_ic < tIC; current_ic++) {
        uint8_t *ic_data = &data[current_ic * 6];
        ic[current_ic].rx_cfgb.dcc = 0x00;
        ic[current_ic].rx_cfgb.gpio = ic_data[0] & 0x0F; // GPIO6-9 bits
        ic[current_ic].rx_cfgb.dcc = (ic_data[0] & 0xF0) >> 4;

        ic[current_ic].rx_cfgb.mute = (ic_data[1] & 0x80) >> 7;
        ic[current_ic].rx_cfgb.fdrf = (ic_data[1] & 0x40) >> 6;
        ic[current_ic].rx_cfgb.ps = (ic_data[1] & 0x30) >> 4;
        ic[current_ic].rx_cfgb.dtmen = (ic_data[1] & 0x08) >> 3;
        ic[current_ic].rx_cfgb.dcc |= (ic_data[1] & 0x03) << 4;
    }
}

void parseConfig(uint8_t tIC, cell_asic_ *ic, GRP grp, uint8_t *data)
{
    parseConfigA(tIC, ic, grp, data);
    parseConfigB(tIC, ic, grp, data + (tIC * 6));
}

void parseCellVoltage(uint8_t tIC, cell_asic_ *ic, GRP grp, uint8_t *cv_data)
{
    uint16_t parsed_cell;
    uint8_t data_counter = 0;

    for (uint8_t current_ic = 0; current_ic < tIC; current_ic++) {
        for (uint8_t current_cell = 0; current_cell < 3; current_cell++) {
            parsed_cell = cv_data[data_counter] | (cv_data[data_counter + 1] << 8);

            switch(grp) {
                case A: // Cells 1-3
                    break;
                case B: // Cells 4-6
                    ic[current_ic].cell.c_codes[current_cell] = parsed_cell;
                    ic[current_ic].cell.c_codes[current_cell + 3] = parsed_cell;
                    break;
                case C: // Cells 7-9
                    ic[current_ic].cell.c_codes[current_cell + 6] = parsed_cell;
                    break;
                case D: // Cells 10-12
                    ic[current_ic].cell.c_codes[current_cell + 9] = parsed_cell;
                    break;
                case E: // Cells 13-15
                    ic[current_ic].cell.c_codes[current_cell + 12] = parsed_cell;
                    break;
                case F: // Cells 16-18
                    ic[current_ic].cell.c_codes[current_cell + 15] = parsed_cell;
                    break;
                case ALL_GRP:
                    // Parse all groups sequentially
                    if (current_cell < 18) {
                        ic[current_ic].cell.c_codes[current_cell] = parsed_cell;
                    }
                    break;
                default:
                    break;
            }
            data_counter += 2;
        }

        if (grp != ALL_GRP) {
            // Skip to next IC data if not reading all groups
            data_counter = (current_ic + 1) * 6;
        }
    }
}

void parseAuxVoltage(uint8_t tIC, cell_asic_ *ic, GRP grp, uint8_t *aux_data)
{
    uint16_t parsed_aux;
    uint8_t data_counter = 0;

    for (uint8_t current_ic = 0; current_ic < tIC; current_ic++) {
        switch(grp) {
            case A: // GPIO1-3
                for (uint8_t current_aux = 0; current_aux < 3; current_aux++) {
                    parsed_aux = aux_data[data_counter] | (aux_data[data_counter + 1] << 8);
                    ic[current_ic].aux.a_codes[current_aux] = parsed_aux;
                    data_counter += 2;
                }
                break;

            case B: // GPIO4-6 + REF
                for (uint8_t current_aux = 3; current_aux < 6; current_aux++) {
                    parsed_aux = aux_data[data_counter] | (aux_data[data_counter + 1] << 8);
                    ic[current_ic].aux.a_codes[current_aux] = parsed_aux;
                    data_counter += 2;
                }
                break;

            case C: // GPIO7-9
                for (uint8_t current_aux = 6; current_aux < 9; current_aux++) {
                    parsed_aux = aux_data[data_counter] | (aux_data[data_counter + 1] << 8);
                    ic[current_ic].aux.a_codes[current_aux] = parsed_aux;
                    data_counter += 2;
                }
                break;

            case D: // REF
                parsed_aux = aux_data[data_counter] | (aux_data[data_counter + 1] << 8);
                ic[current_ic].aux.ref = parsed_aux;
                data_counter += 2;
                break;

            case ALL_GRP:
                for (uint8_t current_aux = 0; current_aux < 9; current_aux++) {
                    parsed_aux = aux_data[data_counter] | (aux_data[data_counter + 1] << 8);
                    ic[current_ic].aux.a_codes[current_aux] = parsed_aux;
                    data_counter += 2;
                }
                parsed_aux = aux_data[data_counter] | (aux_data[data_counter + 1] << 8);
                ic[current_ic].aux.ref = parsed_aux;
                data_counter += 2;
                break;

            default:
                break;
        }

        if (grp != ALL_GRP) {
            data_counter = (current_ic + 1) * 6;
        }
    }
}

void parseStatus(uint8_t tIC, cell_asic_ *ic, GRP grp, uint8_t *stat_data)
{
    uint8_t data_counter = 0;

    for (uint8_t current_ic = 0; current_ic < tIC; current_ic++) {
        switch(grp) {
            case A: // SC, ITMP, VA
                ic[current_ic].stata.sc = stat_data[data_counter] | (stat_data[data_counter + 1] << 8);
                data_counter += 2;
                ic[current_ic].stata.itmp = stat_data[data_counter] | (stat_data[data_counter + 1] << 8);
                data_counter += 2;
                ic[current_ic].stata.va = stat_data[data_counter] | (stat_data[data_counter + 1] << 8);
                data_counter += 2;
                break;

            case B: // VD, OV/UV flags, status bits
                ic[current_ic].statb.vd = stat_data[data_counter] | (stat_data[data_counter + 1] << 8);
                data_counter += 2;

                for (uint8_t i = 0; i < 18; i++) {
                    uint8_t byte_index = data_counter + (i / 4);
                    uint8_t bit_offset = (i % 4) * 2;
                    ic[current_ic].statb.c_uv[i] = (stat_data[byte_index] >> bit_offset) & 0x01;
                    ic[current_ic].statb.c_ov[i] = (stat_data[byte_index] >> (bit_offset + 1)) & 0x01;
                }
                data_counter += 3;

                ic[current_ic].statb.thsd = stat_data[data_counter] & 0x01;
                ic[current_ic].statb.muxfail = (stat_data[data_counter] & 0x02) >> 1;
                ic[current_ic].statb.rev = (stat_data[data_counter] & 0xF0) >> 4;
                data_counter += 1;
                break;

            case ALL_GRP:
                parseStatus(tIC, ic, A, stat_data);
                parseStatus(tIC, ic, B, stat_data + (tIC * 6));
                return;

            default:
                break;
        }

        if (grp != ALL_GRP) {
            data_counter = (current_ic + 1) * 6;
        }
    }
}

void createConfigA(uint8_t tIC, cell_asic_ *ic)
{
    for (uint8_t current_ic = 0; current_ic < tIC; current_ic++) {
        ic[current_ic].ic_register.tx_data[0] = ic[current_ic].tx_cfga.adcopt |
                                                (ic[current_ic].tx_cfga.dten << 1) |
                                                (ic[current_ic].tx_cfga.refon << 2) |
                                                (ic[current_ic].tx_cfga.gpio << 3);

        ic[current_ic].ic_register.tx_data[1] = ic[current_ic].tx_cfga.vuv & 0xFF;
        ic[current_ic].ic_register.tx_data[2] = ((ic[current_ic].tx_cfga.vuv & 0xF00) >> 8) |
                                                ((ic[current_ic].tx_cfga.vov & 0xF) << 4);
        ic[current_ic].ic_register.tx_data[3] = (ic[current_ic].tx_cfga.vov & 0xFF0) >> 4;
        ic[current_ic].ic_register.tx_data[4] = ic[current_ic].tx_cfga.dcc & 0xFF;
        ic[current_ic].ic_register.tx_data[5] = ((ic[current_ic].tx_cfga.dcc & 0xF00) >> 8) |
                                                (ic[current_ic].tx_cfga.dcto << 4);
    }
}

void createConfigB(uint8_t tIC, cell_asic_ *ic)
{
    for (uint8_t current_ic = 0; current_ic < tIC; current_ic++) {
        ic[current_ic].ic_register.tx_data[0] = ic[current_ic].tx_cfgb.gpio |
                                                ((ic[current_ic].tx_cfgb.dcc & 0x0F) << 4);

        ic[current_ic].ic_register.tx_data[1] =  ((ic[current_ic].tx_cfgb.dcc & 0x30) >> 4) |
                                                (ic[current_ic].tx_cfgb.dtmen << 3) |
                                                (ic[current_ic].tx_cfgb.ps << 4) |
                                                (ic[current_ic].tx_cfgb.fdrf << 6) |
                                                (ic[current_ic].tx_cfgb.mute << 7);

        // Clear remaining bytes
        for (uint8_t i = 2; i < 6; i++) {
            ic[current_ic].ic_register.tx_data[i] = 0x00;
        }
    }
}

void createConfig(uint8_t tIC, cell_asic_ *ic)
{
    createConfigA(tIC, ic);
    createConfigB(tIC, ic);
}

void parseComm(uint8_t tIC, cell_asic_ *ic, uint8_t *comm_data) {
    for (uint8_t current_ic = 0; current_ic < tIC; current_ic++) {
        uint8_t *ic_data = &comm_data[current_ic * 6];

        // ICOM0 ve D0 parse et
        ic[current_ic].comm.icomm[0] = (ic_data[0] & 0xF0) >> 4;
        ic[current_ic].comm.data[0] = ((ic_data[0] & 0x0F) << 4) | ((ic_data[1] & 0xF0) >> 4);
        ic[current_ic].comm.fcomm[0] = ic_data[1] & 0x0F;

        // ICOM1 ve D1 parse et
        ic[current_ic].comm.icomm[1] = (ic_data[2] & 0xF0) >> 4;
        ic[current_ic].comm.data[1] = ((ic_data[2] & 0x0F) << 4) | ((ic_data[3] & 0xF0) >> 4);
        ic[current_ic].comm.fcomm[1] = ic_data[3] & 0x0F;

        // ICOM2 ve D2 parse et
        ic[current_ic].comm.icomm[2] = (ic_data[4] & 0xF0) >> 4;
        ic[current_ic].comm.data[2] = ((ic_data[4] & 0x0F) << 4) | ((ic_data[5] & 0xF0) >> 4);
        ic[current_ic].comm.fcomm[2] = ic_data[5] & 0x0F;

        // Raw data'yı da sakla
        for (uint8_t i = 0; i < 6; i++) {
            ic[current_ic].ic_register.rx_data[i] = ic_data[i];
        }
    }
}

void createComm(uint8_t tIC, cell_asic_ *ic) {
    for (uint8_t current_ic = 0; current_ic < tIC; current_ic++) {
        // COMM0 register
        ic[current_ic].ic_register.tx_data[0] = (ic[current_ic].comm.icomm[0] << 4) |
                                                 ((ic[current_ic].comm.data[0] & 0xF0) >> 4);

        // COMM1 register
        ic[current_ic].ic_register.tx_data[1] = ((ic[current_ic].comm.data[0] & 0x0F) << 4) |
                                                 (ic[current_ic].comm.fcomm[0] & 0x0F);

        // COMM2 register
        ic[current_ic].ic_register.tx_data[2] = (ic[current_ic].comm.icomm[1] << 4) |
                                                 ((ic[current_ic].comm.data[1] & 0xF0) >> 4);

        // COMM3 register
        ic[current_ic].ic_register.tx_data[3] = ((ic[current_ic].comm.data[1] & 0x0F) << 4) |
                                                 (ic[current_ic].comm.fcomm[1] & 0x0F);

        // COMM4 register
        ic[current_ic].ic_register.tx_data[4] = (ic[current_ic].comm.icomm[2] << 4) |
                                                 ((ic[current_ic].comm.data[2] & 0xF0) >> 4);

        // COMM5 register
        ic[current_ic].ic_register.tx_data[5] = ((ic[current_ic].comm.data[2] & 0x0F) << 4) |
                                                 (ic[current_ic].comm.fcomm[2] & 0x0F);
    }
}

void setVoltageThresholds(cell_asic_ *ic, float uv_voltage, float ov_voltage)
{
    ic->tx_cfga.vuv = setUnderVoltageLimit(uv_voltage);
    ic->tx_cfga.vov = setOverVoltageLimit(ov_voltage);
}

void initializeGPIO(cell_asic_ *ic)
{
    ic->tx_cfga.gpio = 0;
    setCfgB_GPIO6to9(ic, 0x00);
}

const char* getGPIOStateString(uint8_t state)
{
    return (state == 0) ? "ON" : "OFF";
}

void setAllGPIO(cell_asic_ *ic, uint8_t value)
{
    ic->tx_cfga.gpio = (value & 0x1F);
    setCfgB_GPIO6to9(ic, (value >> 5) & 0x0F);
}

void setCellDischarge(cell_asic_ *ic, uint8_t cell, uint8_t state)
{
    // gerek yok baba.
    // if (cell < 1 || cell > 18) return;

    // if (cell <= 12) {
    //     // Cells 1-12 in Config A
    //     if (state) {
    //         ic->tx_cfga.dcc |= (1 << (cell - 1));
    //     } else {
    //         ic->tx_cfga.dcc &= ~(1 << (cell - 1));
    //     }
    // } else {
    //     // Cells 13-18 in Config B
    //     uint16_t dcc_mask = 0;
    //     if (ic->tx_cfgb.dcc13) dcc_mask |= 0x01;
    //     if (ic->tx_cfgb.dcc14) dcc_mask |= 0x02;
    //     if (ic->tx_cfgb.dcc15) dcc_mask |= 0x04;
    //     if (ic->tx_cfgb.dcc16) dcc_mask |= 0x08;
    //     if (ic->tx_cfgb.dcc17) dcc_mask |= 0x10;
    //     if (ic->tx_cfgb.dcc18) dcc_mask |= 0x20;

    //     if (state) {
    //         dcc_mask |= (1 << (cell - 13));
    //     } else {
    //         dcc_mask &= ~(1 << (cell - 13));
    //     }

    //     setCfgB_DCC13to18(ic, dcc_mask);
    // }
}

