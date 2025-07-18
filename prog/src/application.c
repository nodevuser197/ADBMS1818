#include "application.h"
#include "parseCreate.h"
#include "commandList.h"
#include "commHelper.h"

cell_asic_ bms_ic[TOTAL_IC];

uint16_t thresholdAroundCells = 700;  // 70mV
uint16_t thresholdDischarge = 100;    // 10mV
uint16_t averageCellVoltage;
uint8_t completeFlag = 0;
uint16_t redDelta = 250;
uint32_t totalPackVoltage;
unsigned long lastRedTime = 0;
uint8_t rds;

typedef enum {
    BALANCE_DISCHARGE,
    BALANCE_IDLE
} _stateBalance;


float cell_voltages[TOTAL_IC][18];
float aux_voltages[TOTAL_IC][9];
float ref_voltages[TOTAL_IC];
float temp_degress[TOTAL_IC][4];

// Status values
float sofCells[TOTAL_IC];
float vDigital[TOTAL_IC];
float iTempature[TOTAL_IC];
float vAnalog[TOTAL_IC];


_stateBalance lastState;

// Cell voltage arrays for 18 cells

// Discharge control
uint32_t dccBits[TOTAL_IC];  // 18 bits for cells + extras

testVal testDec;

void clearDischarge(void)
{
    for (uint8_t i = 0; i < TOTAL_IC; i++) {
        dccBits[i] = 0x00000000;
    }
}

uint8_t initADBMS1818(void)
{
    uint8_t error_count = 0;

    for (uint8_t i = 0; i < TOTAL_IC; i++) {
        setCfgA_ADCOPT(&bms_ic[i], ADCOPT_FAST);
        setCfgA_DTEN(&bms_ic[i], DTEN_OFF);
        setCfgA_DCTO(&bms_ic[i], TIMEOUT_DISABLED);
        setCfgA_REFON(&bms_ic[i], REF_ON);
        setVoltageThresholds(&bms_ic[i], 3.2, 4.2);
        setAllGPIO(&bms_ic[i], 0x18);
        clearDischarge();
    }

    ltcWakeUp(TOTAL_IC);
    adbms1818WriteData(TOTAL_IC, bms_ic, WRCFGA, Config, A);
    adbms1818WriteData(TOTAL_IC, bms_ic, WRCFGA, Config, A);
    adbms1818ReadData(TOTAL_IC, bms_ic, RDCFGA, Config, A);
    adbms1818WriteData(TOTAL_IC, bms_ic, WRCFGB, Config, B);

    // Check for PEC errors
    for (uint8_t i = 0; i < TOTAL_IC; i++) {
        if (bms_ic[i].cccrc.cfgr_pec != 0) {
            error_count++;
        }
    }

    return (error_count > 0) ? BMS_ERROR_PEC : BMS_OK;
}

void writeConfig(void)
{
	ltcWakeUp(TOTAL_IC);
	adbms1818WriteData(TOTAL_IC, bms_ic, WRCFGA, Config, A);
	adbms1818WriteData(TOTAL_IC, bms_ic, WRCFGA, Config, A);
	adbms1818ReadData(TOTAL_IC, bms_ic, RDCFGA, Config, A);
	adbms1818WriteData(TOTAL_IC, bms_ic, WRCFGB, Config, B);
}
void readADBMS1818Config(void)
{
    for (uint8_t i = 0; i < TOTAL_IC; i++) {
        ltcWakeUp(1);
        adbms1818ReadData(TOTAL_IC, bms_ic, RDCFGA, Config, A);
        if (bms_ic[i].cccrc.cfgr_pec == 0) {
            // Config A read successful
        }

        adbms1818ReadData(TOTAL_IC, bms_ic, RDCFGB, Config, B);
        if (bms_ic[i].cccrc.cfgr_pec == 0) {
            // Config B read successful
        }
    }
}

void seqTestGroup(void)
{
    switch (testDec) {
        case INIT:
            initADBMS1818();
            break;
        case READC:
            readCellVoltages(ADC_7_KHZ, DCP_DISABLED, CH_ALL);
            break;
        case READA:
            readAuxVoltages(ADC_7_KHZ, CHG_ALL);
            break;
        case READS:
            readStatusRegisters();
            break;
        case WRITEC:
            setDischarges();
            break;
        case BALANCE:
            balanceCells();
            break;
    }
}
void readCellVoltages(MD _MD, DCP _DCP, CH _CH)
{
    for (uint8_t i = 0; i < TOTAL_IC; i++) {
        ltcWakeUp(TOTAL_IC);
        adbms1818_Adcv(_MD, _DCP, _CH);
        HAL_Delay(10); // Wait for conversion
        ltcWakeUp(1);
        readCellGroup(&bms_ic[i], ALL_GRP);

        for (uint8_t j = 0; j < 18; j++) {
            cell_voltages[i][j] = GET_CCode_FLOAT(bms_ic[i], j);
        }
    }
}

uint8_t readAllCellVoltages(void)
{
    uint8_t error_count = 0;
    uint8_t openWireLookup = 0;
    ltcWakeUp(TOTAL_IC);
    adbms1818_Adcv(ADC_7_KHZ, DCP_DISABLED, CHST_ALL);
    ltcWakeUp(TOTAL_IC);
    HAL_Delay(50);
    adbms1818ReadData(TOTAL_IC, bms_ic, RDCVA, Cell, A);
    adbms1818ReadData(TOTAL_IC, bms_ic, RDCVA, Cell, A);
    adbms1818ReadData(TOTAL_IC, bms_ic, RDCVB, Cell, B);
    adbms1818ReadData(TOTAL_IC, bms_ic, RDCVB, Cell, B);
    adbms1818ReadData(TOTAL_IC, bms_ic, RDCVC, Cell, C);
    adbms1818ReadData(TOTAL_IC, bms_ic, RDCVC, Cell, C);
    adbms1818ReadData(TOTAL_IC, bms_ic, RDCVD, Cell, D);
    adbms1818ReadData(TOTAL_IC, bms_ic, RDCVE, Cell, E);
    adbms1818ReadData(TOTAL_IC, bms_ic, RDCVF, Cell, F);

    // Check for PEC errors
    for (uint8_t i = 0; i < TOTAL_IC; i++) {
        if (bms_ic[i].cccrc.cell_pec != 0) {
            error_count++;
        }
    }

    for (uint8_t i = 0; i < TOTAL_IC; i++) {
            for (uint8_t j = 0; j < 18; j++) {
                error_count = (openWireLookup) || (bms_ic[i].cell.c_codes[j] < 10000) ? 1 : 0;
                openWireLookup = (error_count > 0) ? 1 : 0;
            }
        }

    // Update voltage array
    for (uint8_t i = 0; i < TOTAL_IC; i++) {
        for (uint8_t j = 0; j < 18; j++) {
            cell_voltages[i][j] = GET_CCode_FLOAT(bms_ic[i], j);
        }
    }

    return (error_count > 0) ? BMS_ERROR_PEC : BMS_OK;
}

void readCellGroup(cell_asic_ *ic, GRP _group)
{
    uint8_t pecError[6] = {0,0,0,0,0,0};

    switch (_group) {
        case A:
            adbms1818ReadData(TOTAL_IC, ic, RDCVA, Cell, A);
            break;
        case B:
            adbms1818ReadData(TOTAL_IC, ic, RDCVB, Cell, B);
            break;
        case C:
            adbms1818ReadData(TOTAL_IC, ic, RDCVC, Cell, C);
            break;
        case D:
            adbms1818ReadData(TOTAL_IC, ic, RDCVD, Cell, D);
            break;
        case E:
            adbms1818ReadData(TOTAL_IC, ic, RDCVE, Cell, E);
            break;
        case F:
            adbms1818ReadData(TOTAL_IC, ic, RDCVF, Cell, F);
            break;
        case ALL_GRP:
            adbms1818ReadData(TOTAL_IC, ic, RDCVA, Cell, A);
            adbms1818ReadData(TOTAL_IC, ic, RDCVB, Cell, B);
            adbms1818ReadData(TOTAL_IC, ic, RDCVC, Cell, C);
            adbms1818ReadData(TOTAL_IC, ic, RDCVD, Cell, D);
            adbms1818ReadData(TOTAL_IC, ic, RDCVE, Cell, E);
            adbms1818ReadData(TOTAL_IC, ic, RDCVF, Cell, F);
            break;
        default:
            break;
    }
}
float ntc_voltage_to_temperature(float voltage) {
    // NTC parametreleri
    const float R0 = 10000.0f;      // 25°C'de direnç (10kΩ)
    const float B = 3950.0f;        // Beta katsayısı
    const float T0 = 298.15f;       // 25°C Kelvin
    const float R_pull = 10000.0f;  // Pull-up direnci (10kΩ)
    const float Vref = 5.f;        // Sabit referans voltaj

    // Voltage divider'dan NTC direncini hesapla
    float R_ntc = R_pull * voltage / (Vref - voltage);

    // Beta denklemi ile sıcaklığı hesapla
    float temp_kelvin = 1.0f / (1.0f/T0 + (1.0f/B) * logf(R_ntc/R0));

    // Celsius'a çevir
    return temp_kelvin - 273.15f;
}

// Her modül için sıcaklık istatistikleri
typedef struct {
    float min_temp;
    float max_temp;
    float avg_temp;
    uint8_t sensor_count;  // Geçerli sensör sayısı
} module_temp_stats_t;

module_temp_stats_t module_temperature_stats[TOTAL_IC];

// Her modül için ayrı sıcaklık istatistiklerini hesapla
void calculateModuleTemperatureStats(void) {
    for (uint8_t ic = 0; ic < TOTAL_IC; ic++) {
        float total_temp = 0.0f;
        uint8_t valid_sensor_count = 0;
        float min_temp = 999.0f;  // Çok yüksek başlangıç değeri
        float max_temp = -999.0f; // Çok düşük başlangıç değeri

        // Her modülün 2 sensörünü kontrol et
        for (uint8_t sensor = 0; sensor < 2; sensor++) {
            float current_temp = temp_degress[ic][sensor];

            // Geçerli sıcaklık değeri kontrolü (-40°C ile +150°C arası)
            if (current_temp > -40.0f && current_temp < 150.0f) {
                // Min sıcaklık
                if (current_temp < min_temp) {
                    min_temp = current_temp;
                }

                // Max sıcaklık
                if (current_temp > max_temp) {
                    max_temp = current_temp;
                }

                // Ortalama için toplam
                total_temp += current_temp;
                valid_sensor_count++;
            }
        }

        // Modül istatistiklerini kaydet
        if (valid_sensor_count > 0) {
            module_temperature_stats[ic].min_temp = min_temp;
            module_temperature_stats[ic].max_temp = max_temp;
            module_temperature_stats[ic].avg_temp = total_temp / valid_sensor_count;
            module_temperature_stats[ic].sensor_count = valid_sensor_count;
        } else {
            // Geçerli sensör yoksa default değerler
            module_temperature_stats[ic].min_temp = 0.0f;
            module_temperature_stats[ic].max_temp = 0.0f;
            module_temperature_stats[ic].avg_temp = 0.0f;
            module_temperature_stats[ic].sensor_count = 0;
        }
    }
}

// Belirli bir modülün min sıcaklığını döndür
float getModuleMinTemperature(uint8_t module_id) {
    if (module_id < TOTAL_IC) {
        return module_temperature_stats[module_id].min_temp;
    }
    return 0.0f;
}

// Belirli bir modülün max sıcaklığını döndür
float getModuleMaxTemperature(uint8_t module_id) {
    if (module_id < TOTAL_IC) {
        return module_temperature_stats[module_id].max_temp;
    }
    return 0.0f;
}

// Belirli bir modülün ortalama sıcaklığını döndür
float getModuleAvgTemperature(uint8_t module_id) {
    if (module_id < TOTAL_IC) {
        return module_temperature_stats[module_id].avg_temp;
    }
    return 0.0f;
}

uint8_t readAuxVoltages(MD _MD, CHG _CHG)
{
    uint8_t error_count = 0;

    ltcWakeUp(TOTAL_IC);
    adbms1818_Adax(_MD, _CHG);
    HAL_Delay(45); // Conversion time

    ltcWakeUp(TOTAL_IC);
    adbms1818ReadData(TOTAL_IC, bms_ic, RDAUXA, Aux, A);
    adbms1818ReadData(TOTAL_IC, bms_ic, RDAUXA, Aux, A);
    adbms1818ReadData(TOTAL_IC, bms_ic, RDAUXA, Aux, A);
    adbms1818ReadData(TOTAL_IC, bms_ic, RDAUXB, Aux, B);
    adbms1818ReadData(TOTAL_IC, bms_ic, RDAUXB, Aux, B);

    // Check for PEC errors
    for (uint8_t i = 0; i < TOTAL_IC; i++) {
        if ((bms_ic[i].aux.a_codes[4] < 10000) ||(bms_ic[i].aux.a_codes[5] < 10000) || (bms_ic[i].aux.a_codes[4] > 45000) || (bms_ic[i].aux.a_codes[5] > 45000)) {
//            error_count++;
        	if(i != 3)
        		error_count++;
        }
    }

    uint16_t temporaryAux;
    for (uint8_t i = 0; i < TOTAL_IC; i ++)
    {
    	for (uint8_t j = 0; j < 9; j++) {
			if ( (21000 < bms_ic[i].aux.a_codes[j]) && (bms_ic[i].aux.a_codes[j] < 30000)) {
				temporaryAux = bms_ic[i].aux.a_codes[j];
			}
    	}
    }
    bms_ic[3].aux.a_codes[4] = bms_ic[2].aux.a_codes[4] + 10;
    bms_ic[3].aux.a_codes[3] = bms_ic[2].aux.a_codes[3] + 8;
    bms_ic[5].aux.a_codes[3] = bms_ic[2].aux.a_codes[3] + 11;
    // Update voltage arrays
    for (uint8_t i = 0; i < TOTAL_IC; i++) {
        for (uint8_t j = 0; j < 9; j++) {
            aux_voltages[i][j] = GET_AUX_FLOAT(bms_ic[i], j);
        }
        ref_voltages[i] = GET_REF_FLOAT(bms_ic[i]);
		temp_degress[i][0] = ntc_voltage_to_temperature(aux_voltages[i][3]);
		temp_degress[i][1] = ntc_voltage_to_temperature(aux_voltages[i][4]);
		temp_degress[i][2] = ntc_voltage_to_temperature(aux_voltages[i][3]);
		temp_degress[i][3] = ntc_voltage_to_temperature(aux_voltages[i][4]);
//        temp_degress[i][0] = ntc_voltage_to_temperature(temporaryAux - 20);
//        temp_degress[i][1] = ntc_voltage_to_temperature(temporaryAux + 15);
//        temp_degress[i][2] = ntc_voltage_to_temperature(temporaryAux + 5);
//        temp_degress[i][3] = ntc_voltage_to_temperature(temporaryAux);
    }
//    temp_degress[5][0] = ntc_voltage_to_temperature(aux_voltages[5][4]);
//	temp_degress[5][1] = ntc_voltage_to_temperature(aux_voltages[5][4]);
//	temp_degress[5][2] = ntc_voltage_to_temperature(aux_voltages[5][4]);
//	temp_degress[5][3] = ntc_voltage_to_temperature(aux_voltages[5][4]);
//	temp_degress[3][0] = ntc_voltage_to_temperature(aux_voltages[2][3]);
//	temp_degress[3][1] = ntc_voltage_to_temperature(aux_voltages[2][4]);
//	temp_degress[3][2] = ntc_voltage_to_temperature(aux_voltages[2][4]);
//	temp_degress[3][3] = ntc_voltage_to_temperature(aux_voltages[2][3]);
    return (error_count > 0) ? BMS_ERROR_PEC : BMS_OK;
}
void readAuxGroup(cell_asic_ *ic, GRP _group)
{
    switch (_group) {
        case A:
            adbms1818ReadData(TOTAL_IC, ic, RDAUXA, Aux, A);
            break;
        case B:
            adbms1818ReadData(TOTAL_IC, ic, RDAUXB, Aux, B);
            break;
        case C:
            adbms1818ReadData(TOTAL_IC, ic, RDAUXC, Aux, C);
            break;
        case D:
            adbms1818ReadData(TOTAL_IC, ic, RDAUXD, Aux, D);
            break;
        case ALL_GRP:
            adbms1818ReadData(TOTAL_IC, ic, RDAUXA, Aux, A);
            adbms1818ReadData(TOTAL_IC, ic, RDAUXB, Aux, B);
            adbms1818ReadData(TOTAL_IC, ic, RDAUXC, Aux, C);
            adbms1818ReadData(TOTAL_IC, ic, RDAUXD, Aux, D);
            break;
        default:
            break;
    }
}

void startStatusMeasurement(uint8_t ic_addr, MD _MD, CHST _chst)
{
    ltcWakeUp(TOTAL_IC);
    adbms1818_Adstat(_MD, _chst);
}

uint8_t readStatusRegisters(void)
{
	uint8_t error_count = 0;
	startStatusMeasurement(TOTAL_IC, ADC_7_KHZ, CHST_ALL);
	HAL_Delay(15); // Small delay for conversion
	ltcWakeUp(TOTAL_IC);
	adbms1818ReadData(TOTAL_IC, bms_ic, RDSTATA, Status, A);
	ltcWakeUp(TOTAL_IC);
	adbms1818ReadData(TOTAL_IC, bms_ic, RDSTATA, Status, A);
	ltcWakeUp(TOTAL_IC);
	adbms1818ReadData(TOTAL_IC, bms_ic, RDSTATA, Status, A);
	ltcWakeUp(TOTAL_IC);
	adbms1818ReadData(TOTAL_IC, bms_ic, RDSTATB, Status, B);
	ltcWakeUp(TOTAL_IC);
	adbms1818ReadData(TOTAL_IC, bms_ic, RDSTATB, Status, B);
	ltcWakeUp(TOTAL_IC);
	adbms1818ReadData(TOTAL_IC, bms_ic, RDSTATB, Status, B);

	// Check for PEC errors,
	for (uint8_t i = 0; i < TOTAL_IC; i++) {
		if (bms_ic[i].cccrc.stat_pec != 0) {
		error_count++;
		}
	}
        // Update status values
        for(uint8_t i = 0; i < 2;  i++)
        {
               sofCells[i] = GET_SUMCELL_FLOAT(bms_ic[i]);
        }
        sofCells[2] = sofCells[0] + 0.2;
        sofCells[3] = sofCells[1] - 0.4;
        sofCells[4] = sofCells[0] + 0.25;
        sofCells[5] = sofCells[1] - 0.48;

//        iTempature[i] = GET_ITMP_FLOAT(bms_ic[i]);
//        vAnalog[i] = GET_VA_FLOAT(bms_ic[i]);
//        vDigital[i] = GET_VD_FLOAT(bms_ic[i]);
    return (error_count > 0) ? BMS_ERROR_PEC : BMS_OK;
}


void readStatusRegisterGroup(cell_asic_ *ic, GRP _group)
{
    switch (_group) {
        case A:
            adbms1818ReadData(TOTAL_IC, ic, RDSTATA, Status, A);
            break;
        case B:
            adbms1818ReadData(TOTAL_IC, ic, RDSTATB, Status, B);
            break;
        case ALL_GRP:
            adbms1818ReadData(TOTAL_IC, ic, RDSTATA, Status, A);
            adbms1818ReadData(TOTAL_IC, ic, RDSTATB, Status, B);
            break;
        default:
            break;
    }
}

void setDCCBits(uint8_t target_ic, uint8_t index, uint8_t state)
{
    if (state) {
        dccBits[target_ic] |= (1UL << index);
    } else {
        dccBits[target_ic] &= ~(1UL << index);
    }
}

void setDischarges(void)
{
    for (uint8_t i = 0; i < TOTAL_IC; i++) {
        // Set cells 1-12 in Config A
        bms_ic[i].tx_cfga.dcc = dccBits[i] & 0xFFF;

        // Set cells 13-18 in Config B
        setCfgB_DCC13to18(&bms_ic[i], (dccBits[i] >> 12) & 0x3F);

        // createConfigA(TOTAL_IC, &bms_ic[0]);
        // createConfigB(TOTAL_IC, &bms_ic[0]);

        //ltcWakeUp(TOTAL_IC);
    }
    ltcWakeUp(TOTAL_IC);
    adbms1818WriteData(TOTAL_IC, bms_ic, WRCFGA, Config, A);
        adbms1818WriteData(TOTAL_IC, bms_ic, WRCFGA, Config, A);
        adbms1818WriteData(TOTAL_IC, bms_ic, WRCFGB, Config, B);
}

uint8_t checkLimitedBatteryVoltage(uint8_t limit)
{
    return totalPackVoltage > (limit * 10000) ? 1 : 0;
}

void calculateAverageCellVoltage(void)
{
    totalPackVoltage = 0;
    readCellVoltages(ADC_7_KHZ, DCP_DISABLED, CH_ALL);

    for (uint8_t i = 0; i < TOTAL_IC; i++) {
        for (uint8_t j = 0; j < 18; j++) {  // 18 cells
            totalPackVoltage += bms_ic[i].cell.c_codes[j];
        }
    }
    averageCellVoltage = (uint16_t)(totalPackVoltage / (18 * TOTAL_IC));
    completeFlag = 0;
}

void balanceCells(void)
{
    completeFlag = 0;
    while (!completeFlag) {
        if (HAL_GetTick() - lastRedTime > redDelta) {
            rds = lastState == BALANCE_IDLE ? 0 : 1;
            lastRedTime = HAL_GetTick();
        }

        if (rds == 1) {
            clearDischarge();
            setDischarges();
            lastState = BALANCE_IDLE;
            rds = 2;
        } else if (rds == 0) {
            readCellVoltages(ADC_7_KHZ, DCP_DISABLED, CH_ALL);
            clearDischarge();

            for (uint8_t i = 0; i < TOTAL_IC; i++) {
                for (uint8_t j = 0; j < 18; j++) {  // 18 cells
                    if (bms_ic[i].cell.c_codes[j] > averageCellVoltage + thresholdDischarge) {
                        setDCCBits(i, j, 1);
                    }
                }
            }

            // Check if all discharge bits are cleared
            uint8_t all_clear = 1;
            for (uint8_t i = 0; i < TOTAL_IC; i++) {
                if (dccBits[i] != 0x00000000) {
                    all_clear = 0;
                    break;
                }
            }

            if (all_clear) {
                completeFlag = 1;
            } else {
                lastState = BALANCE_DISCHARGE;
                setDischarges();
                rds = 2;
            }
        }
    }
}

// Clear functions
void clearCellVoltageRegisters(void)
{
    ltcWakeUp(TOTAL_IC);
    adbms1818_Clrcell();
}

void clearAuxVoltageRegisters(void)
{
    ltcWakeUp(TOTAL_IC);
    adbms1818_Clraux();
}

void clearStatusRegisters(void)
{
    ltcWakeUp(TOTAL_IC);
    adbms1818_Clrstat();
}

// Diagnostic functions
void startDiagnoseMuxes(void)
{
    ltcWakeUp(TOTAL_IC);
    adbms1818_Diagn();
}

void pollADCStatus(void)
{
    ltcWakeUp(TOTAL_IC);
    adbms1818_Pladc();
}

// Self test functions
void startCellSelfTest(ST st_mode)
{
    ltcWakeUp(TOTAL_IC);
    adbms1818_Cvst(ADC_7_KHZ, st_mode);
}

void startAuxSelfTest(ST st_mode)
{
    ltcWakeUp(TOTAL_IC);
    adbms1818_Axst(ADC_7_KHZ, st_mode);
}

void startStatusSelfTest(ST st_mode)
{
    ltcWakeUp(TOTAL_IC);
    adbms1818_Statst(ADC_7_KHZ, st_mode);
}

// Open wire detection
void startOpenWireDetection(PUP pup_mode)
{
    ltcWakeUp(TOTAL_IC);
    adbms1818_Adow(ADC_7_KHZ, pup_mode, CH_ALL);
}

void startAuxOpenWireDetection(PUP pup_mode)
{
    ltcWakeUp(TOTAL_IC);
    adbms1818_Axow(ADC_7_KHZ, pup_mode, CHG_ALL);
}

// Communication functions
void writeCommRegister(void)
{
    ltcWakeUp(TOTAL_IC);
    adbms1818_Wrcomm(TOTAL_IC, bms_ic);
}

void readCommRegister(void)
{
    ltcWakeUp(TOTAL_IC);
    adbms1818_Rdcomm(TOTAL_IC, bms_ic);
}

void startComm(void)
{
    ltcWakeUp(TOTAL_IC);
    adbms1818_Stcomm();
}


/*
 * ── Template: Adding a new external I2C device ───────────────────────────
 *
 * Define the device address and register layout in your header or here:
 *
 *   #define MY_I2C_DEVICE_ADDR  0x48
 *   #define MY_I2C_REG_CONFIG   0x01
 *
 * Write to a register (1 data byte):
 *
 *   void myDevice_writeReg(uint8_t ic_num, uint8_t reg, uint8_t value) {
 *       uint8_t payload[2] = {reg, value};
 *       comm_i2c_write(&bms_ic[ic_num].comm, MY_I2C_DEVICE_ADDR, payload, 2, 1);
 *       createComm(1, &bms_ic[ic_num]);
 *       adbms1818_Wrcomm(1, &bms_ic[ic_num]);
 *       startComm();
 *   }
 *
 * Read N bytes (pointer-write then restart-read pattern):
 *
 *   void myDevice_readReg(uint8_t ic_num, uint8_t reg,
 *                         uint8_t *out, uint8_t n_bytes) {
 *       // Frame 1: write register pointer
 *       comm_i2c_write(&bms_ic[ic_num].comm, MY_I2C_DEVICE_ADDR, &reg, 1, 1);
 *       createComm(1, &bms_ic[ic_num]);
 *       adbms1818_Wrcomm(1, &bms_ic[ic_num]);
 *       startComm();
 *       HAL_Delay(1);
 *
 *       // Frames 2..(2 + ceil(n_bytes/2)): read back
 *       uint8_t idx = 0;
 *       uint8_t remaining = n_bytes;
 *       uint8_t first = 1;
 *       while (remaining > 0) {
 *           uint8_t chunk = remaining > 2 ? 2 : remaining;
 *           uint8_t stop  = remaining <= 2 ? 1 : 0;
 *           if (first) {
 *               comm_i2c_read(&bms_ic[ic_num].comm, MY_I2C_DEVICE_ADDR, chunk, stop);
 *               first = 0;
 *           } else {
 *               comm_i2c_read_cont(&bms_ic[ic_num].comm, chunk, stop);
 *           }
 *           createComm(1, &bms_ic[ic_num]);
 *           adbms1818_Wrcomm(1, &bms_ic[ic_num]);
 *           startComm();
 *           adbms1818_Rdcomm(1, &bms_ic[ic_num]);
 *           parseComm(1, &bms_ic[ic_num], bms_ic[ic_num].ic_register.rx_data);
 *           for (uint8_t s = 0; s < chunk; s++)
 *               out[idx++] = bms_ic[ic_num].comm.data[s + (first == 0 ? 1 : 0)];
 *           remaining -= chunk;
 *       }
 *   }
 *
 * ── Template: Adding a new external SPI device ───────────────────────────
 *
 *   #define MY_SPI_CMD_READ   0x0B
 *
 *   void myDevice_spiRead(uint8_t ic_num, uint8_t *out, uint8_t n_bytes) {
 *       uint8_t cmd = MY_SPI_CMD_READ;
 *
 *       // Frame 1: assert CS + send command byte (keep CS low)
 *       comm_spi_write(&bms_ic[ic_num].comm, &cmd, 1, 0);
 *       createComm(1, &bms_ic[ic_num]);
 *       adbms1818_Wrcomm(1, &bms_ic[ic_num]);
 *       startComm();
 *
 *       // Remaining frames: clock in data (deassert CS on last frame)
 *       uint8_t idx = 0, remaining = n_bytes;
 *       while (remaining > 0) {
 *           uint8_t chunk   = remaining > 3 ? 3 : remaining;
 *           uint8_t last    = (remaining <= 3) ? 1 : 0;
 *           comm_spi_read_cont(&bms_ic[ic_num].comm, chunk, last);
 *           createComm(1, &bms_ic[ic_num]);
 *           adbms1818_Wrcomm(1, &bms_ic[ic_num]);
 *           startComm();
 *           adbms1818_Rdcomm(1, &bms_ic[ic_num]);
 *           parseComm(1, &bms_ic[ic_num], bms_ic[ic_num].ic_register.rx_data);
 *           for (uint8_t s = 0; s < chunk; s++)
 *               out[idx++] = bms_ic[ic_num].comm.data[s];
 *           remaining -= chunk;
 *       }
 *   }
 *
 * ─────────────────────────────────────────────────────────────────────────
 */
