#include "commandList.h"

const uint8_t WRCFGA[2]  = {0x00, 0x01};
const uint8_t WRCFGB[2]  = {0x00, 0x24};
const uint8_t RDCFGA[2]  = {0x00, 0x02};
const uint8_t RDCFGB[2]  = {0x00, 0x26};

const uint8_t RDCVA[2]   = {0x00, 0x04};
const uint8_t RDCVB[2]   = {0x00, 0x06};
const uint8_t RDCVC[2]   = {0x00, 0x08};
const uint8_t RDCVD[2]   = {0x00, 0x0A};
const uint8_t RDCVE[2]   = {0x00, 0x09};
const uint8_t RDCVF[2]   = {0x00, 0x0B};

const uint8_t RDAUXA[2]  = {0x00, 0x0C};
const uint8_t RDAUXB[2]  = {0x00, 0x0E};
const uint8_t RDAUXC[2]  = {0x00, 0x0D};
const uint8_t RDAUXD[2]  = {0x00, 0x0F};

const uint8_t RDSTATA[2] = {0x00, 0x10};
const uint8_t RDSTATB[2] = {0x00, 0x12};

const uint8_t CLRCELL[2] = {0x07, 0x11};
const uint8_t CLRAUX[2]  = {0x07, 0x12};
const uint8_t CLRSTAT[2] = {0x07, 0x13};

const uint8_t PLADC[2]   = {0x07, 0x14};
const uint8_t DIAGN[2]   = {0x07, 0x15};

const uint8_t WRCOMM[2]  = {0x07, 0x21};
const uint8_t RDCOMM[2]  = {0x07, 0x22};
const uint8_t STCOMM[2]  = {0x07, 0x23};

const uint8_t MUTE[2]    = {0x00, 0x28};
const uint8_t UNMUTE[2]  = {0x00, 0x29};
