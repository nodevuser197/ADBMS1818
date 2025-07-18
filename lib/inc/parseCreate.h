#ifndef __PARSECREATE__H__
#define __PARSECREATE__H__

#include "dataSetList.h"
#include <string.h>
#include <math.h>

uint16_t setOverVoltageLimit(float voltage);
uint16_t setUnderVoltageLimit(float voltage);
uint8_t cfgA_Gpio(uint8_t gpio, COFA_GPIO stat);
uint16_t cfgA_DCC(uint16_t dcc, COFA_DCC stat);

void setCfgA_DCTO(cell_asic_ *ic, COFA_DCTO value);
void setCfgA_REFON(cell_asic_ *ic, COFA_REF value);
void setCfgA_DTEN(cell_asic_ *ic, COFA_DTEN value);
void setCfgA_ADCOPT(cell_asic_ *ic, COFA_ADCOPT value);
void setCfgA_SelectedGPIO(cell_asic_ *ic, uint16_t gpios, COFA_GPIO stat);
void setGPIO_FromHex(cell_asic_ *ic, uint8_t gpio, uint8_t hexValue);

void setCfgB_GPIO6to9(cell_asic_ *ic, uint8_t gpio_mask);
void setCfgB_DCC13to18(cell_asic_ *ic, uint16_t dcc_mask);
void setCfgB_PS(cell_asic_ *ic, uint8_t ps_value);
void setCfgB_DTMEN(cell_asic_ *ic, uint8_t dtmen);

void parseConfigA(uint8_t tIC, cell_asic_ *ic, GRP grp, uint8_t *data);
void parseConfigB(uint8_t tIC, cell_asic_ *ic, GRP grp, uint8_t *data);
void parseConfig(uint8_t tIC, cell_asic_ *ic, GRP grp, uint8_t *data);

void parseCellVoltage(uint8_t tIC, cell_asic_ *ic, GRP grp, uint8_t *cv_data);
void parseAuxVoltage(uint8_t tIC, cell_asic_ *ic, GRP grp, uint8_t *aux_data);
void parseStatus(uint8_t tIC, cell_asic_ *ic, GRP grp, uint8_t *stat_data);
void parseComm(uint8_t tIC, cell_asic_ *ic, uint8_t *comm_data);
void parseADOL(uint8_t tIC, cell_asic_ *ic, uint8_t *data);

void createConfigA(uint8_t tIC, cell_asic_ *ic);
void createConfigB(uint8_t tIC, cell_asic_ *ic);
void createConfig(uint8_t tIC, cell_asic_ *ic);
void createComm(uint8_t tIC, cell_asic_ *ic);

void setVoltageThresholds(cell_asic_ *ic, float uv_voltage, float ov_voltage);
void initializeGPIO(cell_asic_ *ic);
const char* getGPIOStateString(uint8_t state);
void setAllGPIO(cell_asic_ *ic, uint8_t value);
void setCellDischarge(cell_asic_ *ic, uint8_t cell, uint8_t state);

#endif
