#ifndef STUB_STM32G4XX_HAL_SPI_H
#define STUB_STM32G4XX_HAL_SPI_H

/* CI stub — replaces the real STM32 HAL SPI driver. */

#include <stdint.h>

typedef struct { uint32_t reserved; } SPI_HandleTypeDef;

typedef uint32_t HAL_StatusTypeDef;
#define HAL_OK 0U

static inline HAL_StatusTypeDef
HAL_SPI_Transmit(SPI_HandleTypeDef *h, uint8_t *d, uint16_t sz, uint32_t t)
    { (void)h; (void)d; (void)sz; (void)t; return HAL_OK; }

static inline HAL_StatusTypeDef
HAL_SPI_Receive(SPI_HandleTypeDef *h, uint8_t *d, uint16_t sz, uint32_t t)
    { (void)h; (void)d; (void)sz; (void)t; return HAL_OK; }

#endif /* STUB_STM32G4XX_HAL_SPI_H */
