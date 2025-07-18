#ifndef STUB_STM32G4XX_HAL_H
#define STUB_STM32G4XX_HAL_H

/* CI stub — replaces the real STM32 HAL so GCC can compile on Linux.
 * All functions are no-ops; no hardware interaction occurs. */

#include <stdint.h>

typedef uint16_t GPIO_PinState;
#define GPIO_PIN_SET   ((GPIO_PinState)1U)
#define GPIO_PIN_RESET ((GPIO_PinState)0U)

typedef struct { uint32_t reserved; } GPIO_TypeDef;
#define GPIOD      ((GPIO_TypeDef *)0x48000C00UL)
#define GPIO_PIN_2 ((uint16_t)0x0004U)

#define HAL_MAX_DELAY 0xFFFFFFFFU

static inline void     HAL_Delay(uint32_t ms)
    { (void)ms; }
static inline uint32_t HAL_GetTick(void)
    { return 0U; }
static inline void     HAL_GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin,
                                          GPIO_PinState state)
    { (void)port; (void)pin; (void)state; }

#define __NOP() do {} while (0)

#endif /* STUB_STM32G4XX_HAL_H */
