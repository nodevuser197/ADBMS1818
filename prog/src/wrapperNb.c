#include "wrapperNb.h"
#include "commNb.h"

/* CPU clock used by the inter-command gap busy-wait. Override with -D if your
 * core runs at a different frequency (STM32G4 default = 170 MHz). */
#ifndef COMM_NB_CPU_HZ
#define COMM_NB_CPU_HZ 170000000u
#endif

/* Define strong HAL SPI callbacks here.  Set to 0 if the application already
 * owns HAL_SPI_Tx/RxCpltCallback and will forward to commNb itself. */
#ifndef COMM_NB_DEFINE_CALLBACKS
#define COMM_NB_DEFINE_CALLBACKS 1
#endif

/* ── Bound configuration ──────────────────────────────────────────────────── */
static SPI_HandleTypeDef *s_hspi;
static GPIO_TypeDef      *s_cs_port;
static uint16_t           s_cs_pin;
static CommNbMode         s_mode;

/* ── Backend primitives ───────────────────────────────────────────────────── */

static void cs_low(void)
{
    HAL_GPIO_WritePin(s_cs_port, s_cs_pin, GPIO_PIN_RESET);
}

static void cs_high(void)
{
    HAL_GPIO_WritePin(s_cs_port, s_cs_pin, GPIO_PIN_SET);
}

static int tx_async(const uint8_t *data, uint16_t len)
{
    HAL_StatusTypeDef st;
    switch (s_mode) {
        case COMM_NB_MODE_DMA:
            st = HAL_SPI_Transmit_DMA(s_hspi, (uint8_t *)data, len);
            break;
        case COMM_NB_MODE_BLOCKING:
            st = HAL_SPI_Transmit(s_hspi, (uint8_t *)data, len, HAL_MAX_DELAY);
            if (st == HAL_OK) commNb_OnTxDone();   /* inline completion */
            return (st == HAL_OK) ? 0 : -1;
        case COMM_NB_MODE_IT:
        default:
            st = HAL_SPI_Transmit_IT(s_hspi, (uint8_t *)data, len);
            break;
    }
    return (st == HAL_OK) ? 0 : -1;
}

static int rx_async(uint8_t *data, uint16_t len)
{
    HAL_StatusTypeDef st;
    switch (s_mode) {
        case COMM_NB_MODE_DMA:
            st = HAL_SPI_Receive_DMA(s_hspi, data, len);
            break;
        case COMM_NB_MODE_BLOCKING:
            st = HAL_SPI_Receive(s_hspi, data, len, HAL_MAX_DELAY);
            if (st == HAL_OK) commNb_OnRxDone();   /* inline completion */
            return (st == HAL_OK) ? 0 : -1;
        case COMM_NB_MODE_IT:
        default:
            st = HAL_SPI_Receive_IT(s_hspi, data, len);
            break;
    }
    return (st == HAL_OK) ? 0 : -1;
}

/* Coarse busy-wait honouring the CSB-high gap (t5/t6). Erring long is safe.
 * ~3 cycles per loop iteration is assumed (conservative). */
static void delay_ns(uint32_t ns)
{
    uint32_t cyc   = (uint32_t)(((uint64_t)ns * (COMM_NB_CPU_HZ / 1000000u)) / 1000u);
    volatile uint32_t iters = (cyc / 3u) + 1u;
    while (iters--) {
        __NOP();
    }
}

static uint32_t now_ms(void)
{
    return HAL_GetTick();
}

static const CommNbBackend s_backend = {
    .cs_low   = cs_low,
    .cs_high  = cs_high,
    .tx_async = tx_async,
    .rx_async = rx_async,
    .delay_ns = delay_ns,
    .now_ms   = now_ms,
};

/* ── Public init ──────────────────────────────────────────────────────────── */

void commNb_stm32_init(SPI_HandleTypeDef *hspi,
                       GPIO_TypeDef *cs_port, uint16_t cs_pin,
                       CommNbMode mode)
{
    s_hspi    = hspi;
    s_cs_port = cs_port;
    s_cs_pin  = cs_pin;
    s_mode    = mode;
    cs_high();                       /* idle the bus */
    commNb_SetBackend(&s_backend);
}

void commNb_stm32_setMode(CommNbMode mode)
{
    s_mode = mode;
}

/* ── Strong HAL callbacks (IT/DMA) ────────────────────────────────────────── */
#if COMM_NB_DEFINE_CALLBACKS

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi == s_hspi) commNb_OnTxDone();
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi == s_hspi) commNb_OnRxDone();
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi == s_hspi) commNb_OnError();
}

#endif /* COMM_NB_DEFINE_CALLBACKS */
