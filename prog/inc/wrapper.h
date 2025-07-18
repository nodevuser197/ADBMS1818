#ifndef __WRAPPER__H__
#define __WRAPPER__H__

#include <stdint.h>
#include "stm32g4xx_hal.h"
#include "stm32g4xx_hal_spi.h"

void initSpiHandle();
void externalDelay(uint32_t delay);

void ltcCSLow(void);
void ltcCSHigh(void);

void spiWriteBytes(uint8_t *data, uint16_t length);
void spiWriteReadBytes(uint8_t tx_Data[], uint8_t tx_len, uint8_t* rx_data, uint8_t rx_len);
void spiReadBytes(uint8_t *data, uint16_t length);

void ltcWakeUp(uint8_t total_ic);

#endif
