
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "stm32g4xx_hal.h"
#include "stm32g4xx_hal_spi.h"


// External SPI handle - should be defined in your main project
extern SPI_HandleTypeDef hspi1;

#define LTC_SPI_TIMEOUT HAL_MAX_DELAY
#define LTC_CS GPIO_PIN_2
#define LTC_CS_PORT GPIOD
#define DELAY_400US_CYCLES 3200

void initSpiHandle()
{
    // GEREK YOK // This function is not needed in this context, as the SPI handle is already defined externally.
    // hspi1.Instance = SPI1;
    // hspi1.Init.Mode = SPI_MODE_MASTER;
    // hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    // hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    // hspi1.Init.CLKPolarity = SPI_POLARITY_HIGH;
    // hspi1.Init.CLKPhase = SPI_PHASE_2EDGE;
    // hspi1.Init.NSS = SPI_NSS_SOFT;
    // hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
    // hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    // hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    // hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    // hspi1.Init.CRCPolynomial = 10;

    // if (HAL_SPI_Init(&hspi1) != HAL_OK) {
    //     while(1) {
    //         // Error handling - stay in loop
    //     }
    // }
}

void externalDelay(uint32_t delay)
{
    HAL_Delay(delay);
}

void ltcCSLow(void)
{
    HAL_GPIO_WritePin(LTC_CS_PORT, LTC_CS, GPIO_PIN_RESET);
}

void ltcCSHigh(void)
{
    HAL_GPIO_WritePin(LTC_CS_PORT, LTC_CS, GPIO_PIN_SET);
}

void spiWriteBytes(uint8_t *data, uint16_t length)
{
    HAL_SPI_Transmit(&hspi1, data, length, LTC_SPI_TIMEOUT);
}

void spiWriteReadBytes(uint8_t tx_Data[], uint8_t tx_len, uint8_t* rx_data, uint8_t rx_len)
{
    // İlk komut gönder
    HAL_SPI_Transmit(&hspi1, tx_Data, tx_len, HAL_MAX_DELAY);

    // Sonra data al
    HAL_SPI_Receive(&hspi1, rx_data, rx_len, HAL_MAX_DELAY);
}

void spiReadBytes(uint8_t *data, uint16_t length)
{
    HAL_SPI_Receive(&hspi1, data, length, LTC_SPI_TIMEOUT);
}

void ltcWakeUp(uint8_t total_ic) {
    for(uint8_t i = 0; i < (total_ic); i++) {
        ltcCSLow();
        for(volatile uint32_t i = 0; i < DELAY_400US_CYCLES; i++) {
               __NOP();
           }
        ltcCSHigh();
//        externalDelay(1);
    }
}
