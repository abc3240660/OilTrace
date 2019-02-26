#include "common.h"
#include "malloc.h"

// u8 w25q_buf[W25Q_SECTOR_SIZE] = {0};

SYS_ENV g_sys_env_old;
SYS_ENV g_sys_env_new;

// 0-not configed
// 1-from spi flash
// 2-from server
extern u8 sys_env_mode;

void sys_env_update(void)
{
	sys_env_mode = 2;
	g_sys_env_new.active_flag = 0x6789;	
	W25QXX_Write((u8*)&g_sys_env_new, ENV_SECTOR_INDEX_ECAR*W25Q_SECTOR_SIZE, sizeof(SYS_ENV));
}

void sys_env_dump(void)
{
	W25QXX_Read((u8*)&g_sys_env_old, ENV_SECTOR_INDEX_ECAR*W25Q_SECTOR_SIZE, sizeof(SYS_ENV));//读出整个扇区的内容
	
	if (0x6789 == g_sys_env_old.active_flag) {
		sys_env_mode = 1;
	} else {
		sys_env_mode = 0;
	}
}
