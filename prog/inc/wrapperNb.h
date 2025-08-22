#ifndef __WRAPPER_NB_H__
#define __WRAPPER_NB_H__

/*
 * STM32 (HAL) backend for the non-blocking COMM engine.
 * ====================================================
 * Ready-made adapter: the user does NOT write any SPI/CS/ISR code — just call
 * commNb_stm32_init() once with their SPI handle, CS pin and desired transfer
 * mode, then use commNb_Start() / frame callbacks from commNb.h.
 *
 * Transfer mode is chosen in code (init argument), not a build flag, so it can
 * even be switched at runtime:
 *
 *     commNb_stm32_init(&hspi1, GPIOD, GPIO_PIN_2, COMM_NB_MODE_IT);   // IRQ
 *     commNb_stm32_init(&hspi1, GPIOD, GPIO_PIN_2, COMM_NB_MODE_DMA);  // DMA
 *     commNb_stm32_init(&hspi1, GPIOD, GPIO_PIN_2, COMM_NB_MODE_BLOCKING);
 *
 * In IT/DMA modes the engine advances from the SPI completion ISR.  This file
 * provides strong HAL_SPI_Tx/Rx/ErrorCallback definitions that forward to the
 * engine — define COMM_NB_DEFINE_CALLBACKS=0 if your project already owns
 * those callbacks (then call commNb_OnTxDone/OnRxDone/OnError yourself).
 *
 * In BLOCKING mode no IRQ is needed; the transfer completes and notifies the
 * engine inline, so the whole transaction finishes inside commNb_Start().
 */

#include <stdint.h>
#include "stm32g4xx_hal.h"
#include "stm32g4xx_hal_spi.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    COMM_NB_MODE_IT       = 0,   /* HAL_SPI_*_IT,  advance from ISR          */
    COMM_NB_MODE_DMA      = 1,   /* HAL_SPI_*_DMA, advance from ISR          */
    COMM_NB_MODE_BLOCKING = 2    /* HAL_SPI_* blocking, advance inline       */
} CommNbMode;

/* Configure and register the STM32 backend with the engine.
 *   hspi    : SPI peripheral handle (e.g. &hspi1)
 *   cs_port : GPIO port of the ADBMS chip-select
 *   cs_pin  : GPIO pin  of the ADBMS chip-select
 *   mode    : COMM_NB_MODE_IT / _DMA / _BLOCKING                            */
void commNb_stm32_init(SPI_HandleTypeDef *hspi,
                       GPIO_TypeDef *cs_port, uint16_t cs_pin,
                       CommNbMode mode);

/* Change transfer mode after init (e.g. fall back to blocking). */
void commNb_stm32_setMode(CommNbMode mode);

#ifdef __cplusplus
}
#endif

#endif /* __WRAPPER_NB_H__ */
