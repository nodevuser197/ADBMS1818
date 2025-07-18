#ifndef __APPLICATION_H__
#define __APPLICATION_H__

#include "genericType.h"
#include "dataSetList.h"
#include "wrapper.h"
#include "commHelper.h"

typedef enum {
    INIT = 0,
    READC,
    READA,
    READS,
    WRITEC,
    BALANCE
} testVal;


#define TOTAL_IC 6

// BMS Error Codes
#define BMS_OK                 0
#define BMS_ERROR_INIT         1
#define BMS_ERROR_SPI          2
#define BMS_ERROR_PEC          3
#define BMS_ERROR_TIMEOUT      4
#define BMS_ERROR_CONFIG       5

#define GET_CCode_FLOAT(icPtr,index) ((float)icPtr.cell.c_codes[index]/10000.0f)
#define GET_AUX_FLOAT(icPtr,index) ((float)icPtr.aux.a_codes[index]/10000.0f)
#define GET_REF_FLOAT(icPtr) ((float)icPtr.aux.ref/10000.0f)
#define GET_SUMCELL_FLOAT(icPtr) (((float)icPtr.stata.sc/10000.0f)*30)
#define GET_ITMP_FLOAT(icPtr) ((((float)icPtr.stata.itmp/10000.0f)/0.0076f)-276)
#define GET_VA_FLOAT(icPtr) ((float)icPtr.stata.va/10000.0f)
#define GET_VD_FLOAT(icPtr) ((float)icPtr.statb.vd/10000.0f)

void seqTestGroup(void);
uint8_t initADBMS1818(void);                    // Changed to return uint8_t
void readADBMS1818Config(void);

void readCellGroup(cell_asic_ *ic, GRP _group);
void readCellVoltages(MD _MD, DCP _DCP, CH _CH);
uint8_t readAllCellVoltages(void);               // Changed to return uint8_t

uint8_t readAuxVoltages(MD _MD, CHG _CHG);       // Changed to return uint8_t
void readAuxGroup(cell_asic_ *ic, GRP _group);

uint8_t readStatusRegisters(void);               // Changed to return uint8_t
void readStatusRegisterGroup(cell_asic_ *ic, GRP _group);
void startStatusMeasurement(uint8_t ic_addr, MD _MD, CHST _chst);

void setDischarges(void);
void clearDischarge(void);
void setDCCBits(uint8_t target_ic, uint8_t index, uint8_t state);

void calculateAverageCellVoltage(void);
uint8_t checkLimitedBatteryVoltage(uint8_t limit);
void balanceCells(void);
void writeConfig(void);

// Communication functions
void writeCommRegister(void);
void readCommRegister(void);
void startComm(void);

// Clear functions
void clearCellVoltageRegisters(void);
void clearAuxVoltageRegisters(void);
void clearStatusRegisters(void);

// Diagnostic functions
void startDiagnoseMuxes(void);
void pollADCStatus(void);

// Self test functions
void startCellSelfTest(ST st_mode);
void startAuxSelfTest(ST st_mode);
void startStatusSelfTest(ST st_mode);

// Open wire detection
void startOpenWireDetection(PUP pup_mode);
void startAuxOpenWireDetection(PUP pup_mode);

float getModuleMaxTemperature(uint8_t module_id);
float getModuleMinTemperature(uint8_t module_id);
float getModuleAvgTemperature(uint8_t module_id);

#endif
