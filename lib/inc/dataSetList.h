#ifndef __DATASETLIST__H__
#define __DATASETLIST__H__

#include <stdint.h>

typedef struct {
    uint8_t gpio : 5;
    uint8_t refon : 1;
    uint8_t dten : 1;
    uint8_t adcopt : 1;
    uint16_t vuv : 12;
    uint16_t vov : 12;
    uint16_t dcc : 12;  // First 12 discharge bits
    uint8_t dcto : 4;
} cfga_;

typedef struct {
    uint8_t gpio : 4;
    uint8_t dcc : 6;
    uint8_t dtmen : 1;
    uint8_t ps : 2;     // Power save mode
    uint8_t fdrf : 1; // Fast discharge rate flag
    uint8_t mute : 1;   // Mute flag
} cfgb_;

typedef struct {
    uint16_t c_codes[18];  // 18 cells instead of 12
} cv_;


typedef struct
{
  uint8_t fcomm[3];
  uint8_t icomm[3];
  uint8_t data[3];
} com_;

typedef struct {
    uint16_t a_codes[9];   // 9 GPIO instead of 5
    uint16_t ref;
} aux_;

typedef struct {
    uint16_t sc;
    uint16_t itmp;
    uint16_t va;
} sta_;

typedef struct {
    uint16_t vd;
    uint8_t c_ov[18];      // 18 overvoltage flags
    uint8_t c_uv[18];      // 18 undervoltage flags
    uint8_t thsd : 1;
    uint8_t muxfail : 1;
    uint8_t rsvd : 2;
    uint8_t rev : 4;
} stb_;

typedef struct {
    uint8_t tx_data[6];
    uint8_t rx_data[8];
} ic_register_;

typedef struct {
    uint8_t fcomm[3];
    uint8_t icomm[3];
    uint8_t data[3];
} comm_;

typedef struct {
    cfga_ tx_cfga;
    cfga_ rx_cfga;
    cfgb_ tx_cfgb;         // Add config B
    cfgb_ rx_cfgb;
    cv_ cell;
    aux_ aux;
    sta_ stata;
    stb_ statb;
    comm_ comm;
    ic_register_ ic_register;
    // Add error tracking
    struct {
        uint8_t cfgr_pec;
        uint8_t cell_pec;
        uint8_t aux_pec;
        uint8_t stat_pec;
        uint8_t comm_pec;
    } cccrc;
} cell_asic_;

// Update enums for 18 cells and 9 GPIO
typedef enum
{
    GPIO_CLR = 0x0,
    GPIO_SET = 0x1
} COFA_GPIO;

typedef enum
{
    REF_OFF = 0x0,
    REF_ON = 0x1
} COFA_REF;

typedef enum
{
    DTEN_OFF = 0x0,
    DTEN_ON = 0x1
} COFA_DTEN;

typedef enum
{
    DTMEN_OFF = 0x0,
    DTMEN_ON = 0x1
} COFB_DTMEN;

typedef enum
{
    ADCOPT_NORMAL = 0x0,
    ADCOPT_FAST = 0x1
} COFA_ADCOPT;

typedef enum
{
    VUV_3_7 = 0x908,
    VUV_2_5 = 0x61A,
} COFA_VUV;

typedef enum
{
    VOV_4_2 = 0xA41,
    VOV_4_0 = 0x9C4
} COFA_VOV;

typedef enum
{
    DCC_OFF = 0x0,
    DCC_ON = 0x1
} COFA_DCC;

typedef enum
{
    TIMEOUT_DISABLED = 0x0,
    TIMEOUT_1MIN = 0x2,
    TIMEOUT_2MIN = 0x3,
    TIMEOUT_3MIN = 0x4,
    TIMEOUT_4MIN = 0x5,
    TIMEOUT_5MIN = 0x6,
    TIMEOUT_10MIN = 0x7,
    TIMEOUT_15MIN = 0x8,
    TIMEOUT_20MIN = 0x9,
    TIMEOUT_30MIN = 0xA,
    TIMEOUT_40MIN = 0xB,
    TIMEOUT_60MIN = 0xC,
    TIMEOUT_75MIN = 0xD,
    TIMEOUT_90MIN = 0xE,
    TIMEOUT_120MIN = 0xF
} COFA_DCTO;

typedef enum
{
    DRIVE_S_HIGH = 0x0,
    SEND_1 = 0x1,
    SEND_2 = 0x2,
    SEND_3 = 0x3,
    SEND_4 = 0x4,
    SEND_5 = 0x5,
    SEND_6 = 0x6,
    SEND_7 = 0x7,
    DRIVE_S_LOW = 0x8,
} SCTRL_SC;

typedef enum{
    FDRF_OFF = 0x0,
    FDRF_ON = 0x1
} COFB_FDRF;

typedef enum {
    MUTE_OFF = 0x0,
    MUTE_ON = 0x1
} COFB_MUTE;

typedef enum {
    PS_SEQ = 0x0,
    PS_ADC1 = 0x1,
    PS_ADC2 = 0x2,
    PS_ADC3 = 0x3
} COFB_PS;
typedef enum
{
    PWM_0_0 = 0x0,
    PWM_6_7 = 0x1,
    PWM_13_3 = 0x2,
    PWM_20_0 = 0x3,
    PWM_26_7 = 0x4,
    PWM_33_3 = 0x5,
    PWM_40_0 = 0x6,
    PWM_46_7 = 0x7,
    PWM_53_3 = 0x8,
    PWM_60_0 = 0x9,
    PWM_66_7 = 0xA,
    PWM_73_3 = 0xB,
    PWM_80_0 = 0xC,
    PWM_86_7 = 0xD,
    PWM_93_3 = 0xE,
    PWM_100_0 = 0xF
} PWM_DUTY;

typedef enum {
    ADC_422_HZ = 0x0,
    ADC_27_KHZ = 0x1,
    ADC_7_KHZ = 0x2,
    ADC_26_HZ = 0x3
} MD;

typedef enum
{
    DCP_DISABLED = 0x0,
    DCP_ENABLED = 0x1
} DCP;


typedef enum {
    CH_ALL = 0x0,
    CH_1_7_13 = 0x1,
    CH_2_8_14 = 0x2,
    CH_3_9_15 = 0x3,
    CH_4_10_16 = 0x4,
    CH_5_11_17 = 0x5,
    CH_6_12_18 = 0x6
} CH;


typedef enum
{
    PULL_DOWN = 0x0,
    PULL_UP = 0x1
} PUP;

typedef enum
{
    SELF_TEST1 = 0x1,
    SELF_TEST2 = 0x2,
} ST;

typedef enum {
    CHG_ALL = 0x0,
    CHG_GPIO1_2 = 0x1,
    CHG_GPIO3_4 = 0x2,
    CHG_GPIO5_6 = 0x3,
    CHG_GPIO7_8 = 0x4,
    CHG_GPIO9 = 0x5,
    CHG_REF = 0x6
} CHG;

typedef enum {
    CHST_ALL = 0x0,
    CHST_SC = 0x1,
    CHST_ITMP = 0x2,
    CHST_VA = 0x3,
    CHST_VD = 0x4
} CHST;

typedef enum {
    ALL_GRP = 0x0,
    A, B, C, D, E, F,  // Add E and F groups
    NONE
} GRP;

typedef enum
{
    /* I2C mode (ICOMn[3] = 0) */
    ISTART   = 0x06,
    ISTOP    = 0x01,
    IBLANK   = 0x00,
    INOTRANS = 0x07,
    /* SPI mode (ICOMn[3] = 1) */
    ISPI_CS_LOW  = 0x08,
    ISPI_CS_HIGH = 0x09,
    ISPI_CS_FALL = 0x0A,
    ISPI_NOTRANS = 0x0F,
} ICOMM;

typedef enum
{
    /* I2C write */
    FMASTER_ACK       = 0x00,
    FMASTER_NACK      = 0x08,
    FMASTER_NACK_STOP = 0x09,
    /* I2C read */
    FSLAVE_ACK        = 0x07,
    FSLAVE_ACK_STOP   = 0x01,
    FSLAVE_NACK       = 0x0F,
    FSLAVE_NACK_STOP  = 0x09,
    /* SPI write/read */
    FSPI_CS_HOLD      = 0x00,
    FSPI_CS_HIGH      = 0x09,
    FSPI_RD           = 0x0F,
} FCOMM;


#endif
