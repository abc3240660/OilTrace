#ifndef __COMMON_H
#define	__COMMON_H

#include <stdio.h>
#include "delay.h"
#include "w25qxx.h" 

// 1*W25Q128 = 256 Block
// 1*Block   = 16*Sector
// 1*Sector  = 4KB
// ---> 1*W25Q128 = 16MB

// Block 0 Sector 0
#define ENV_SECTOR_INDEX_ECAR	0

// Block 1 Sector 0
#define BIN_SECTOR_INDEX_IAP	16

#define W25Q_BLOCK_COUNT	256
#define W25Q_SETOR_COUNT	(256*16)

#define W25Q_SECTOR_SIZE	4096

#define OFFLINE_SIZE_PER	64

#define OFFLINE_OFFSET	W25Q_SECTOR_SIZE

typedef struct {
	u16 active_flag;
	float ran_max[22];
	float ran_min[22];
	float war_max[22];
	float war_min[22];
	u8 war_mode[22];
} SYS_ENV;

typedef struct {
	u8 systime[7];
	u8 mes_val[44];
} OFFLINE_DAT;

void sys_env_dump(void);
void sys_env_update(void);
void offline_write(float *val);
void offline_dump(OFFLINE_DAT *p_off_dat);
void offline_init(void);

#endif /* __COMMON_H */
