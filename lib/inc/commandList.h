#ifndef __COMMANDLIST_H__
#define __COMMANDLIST_H__

#include <stdint.h>

/* Configuration register group */
extern const uint8_t WRCFGA[2];
extern const uint8_t WRCFGB[2];
extern const uint8_t RDCFGA[2];
extern const uint8_t RDCFGB[2];

/* Read cell voltage register group - 6 groups for 18 cells */
extern const uint8_t RDCVA[2];
extern const uint8_t RDCVB[2];
extern const uint8_t RDCVC[2];
extern const uint8_t RDCVD[2];
extern const uint8_t RDCVE[2];
extern const uint8_t RDCVF[2];

/* Read aux result group - 4 groups for 9 GPIO */
extern const uint8_t RDAUXA[2];
extern const uint8_t RDAUXB[2];
extern const uint8_t RDAUXC[2];
extern const uint8_t RDAUXD[2];

/* Read status register group */
extern const uint8_t RDSTATA[2];
extern const uint8_t RDSTATB[2];

/* Clear register group */
extern const uint8_t CLRCELL[2];
extern const uint8_t CLRAUX[2];
extern const uint8_t CLRSTAT[2];

/* Poll & diagnostic commands */
extern const uint8_t PLADC[2];
extern const uint8_t DIAGN[2];

/* Communication commands */
extern const uint8_t WRCOMM[2];
extern const uint8_t RDCOMM[2];
extern const uint8_t STCOMM[2];

/* Mute/Unmute commands */
extern const uint8_t MUTE[2];
extern const uint8_t UNMUTE[2];

#endif
