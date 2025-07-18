#ifndef __GENERICTYPE_H__
#define __GENERICTYPE_H__

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>        // printf için
#include "wrapper.h"
#include "dataSetList.h"

// Memory sizes for ADBMS1818
#define RX_DATA         8   // Standard register size + PEC
#define TX_DATA         6   // Standard data size
#define RDCVALL_SIZE    38  // All cell groups (36 bytes + 2 PEC)
#define RDAUXALL_SIZE   30  // All aux groups (28 bytes + 2 PEC)
#define RDSTATALL_SIZE  14  // All status groups (12 bytes + 2 PEC)

// Function prototypes

typedef enum {
    Config,
    Cell,
    Aux,
    Status,
    Comm,
    Rdcvall,
    Rdauxall,
    Rdstatall
} TYPE;

uint16_t Pec15_Calc(uint8_t len, uint8_t *data);
void spiSendCmd(uint8_t tx_cmd[2]);
void spiSendCmdKeepCS(uint8_t tx_cmd[2]);
void spiReadData(uint8_t tIC, uint8_t tx_cmd[2], uint8_t *rx_data,
                 uint8_t *pec_error, uint8_t regLength);
void spiWriteData(uint8_t tIC, uint8_t tx_cmd[2], uint8_t *data);

// High-level read/write functions
void adbms1818ReadData(uint8_t tIC, cell_asic_ *ic, uint8_t cmd_arg[2],
                       TYPE type, GRP group);
void adbms1818WriteData(uint8_t tIC, cell_asic_ *ic, uint8_t cmd_arg[2],
                        TYPE type, GRP group);

// ADC Commands
void adbms1818_Adcv(MD md, DCP dcp, CH ch);
void adbms1818_Adcvax(MD md, DCP dcp);
void adbms1818_Adcvsc(MD md, DCP dcp);
void adbms1818_Adax(MD md, CHG chg);
void adbms1818_Adstat(MD md, CHST chst);
void adbms1818_Adol(MD md, DCP dcp);

// Open wire detection
void adbms1818_Adow(MD md, PUP pup, CH ch);
void adbms1818_Axow(MD md, PUP pup, CHG chg);

// Self test commands
void adbms1818_Cvst(MD md, ST st);
void adbms1818_Axst(MD md, ST st);
void adbms1818_Statst(MD md, ST st);

// Clear commands
void adbms1818_Clrcell(void);
void adbms1818_Clraux(void);
void adbms1818_Clrstat(void);

// Diagnostic commands
void adbms1818_Diagn(void);
void adbms1818_Pladc(void);

// Communication commands
void adbms1818_Wrcomm(uint8_t tIC, cell_asic_ *ic);
void adbms1818_Rdcomm(uint8_t tIC, cell_asic_ *ic);
void adbms1818_Stcomm(void);

// Mute/Unmute commands
void adbms1818_Mute(void);
void adbms1818_Unmute(void);

// Polling function
uint32_t adbms1818PollAdc(uint8_t tx_cmd[2]);

#endif
