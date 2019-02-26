#include "common.h"
#include "malloc.h"

// u8 w25q_buf[W25Q_SECTOR_SIZE] = {0};

SYS_ENV g_sys_env_old;
SYS_ENV g_sys_env_new;

// 0-not configed
// 1-from spi flash
// 2-from server
extern u8 sys_env_mode;

u32 total_count_64b = 0;
u32 avaliable_count_64b = 0;
u32 read_pos_next_count_64b = 0;
u32 write_pos_next_count_64b = 0;

void offline_init(void)
{
	total_count_64b = (W25Q_SETOR_COUNT-1)*W25Q_SECTOR_SIZE / OFFLINE_SIZE_PER;
	avaliable_count_64b = 0;
	read_pos_next_count_64b = 0;
	write_pos_next_count_64b = 0;
}

void offline_write(OFFLINE_DAT *p_off_dat)
{
	u32 write_addr = 0;
	
	// OSMutexPend();
	write_addr = OFFLINE_OFFSET + (write_pos_next_count_64b%total_count_64b)*OFFLINE_SIZE_PER;
	
	W25QXX_Write((u8*)&p_off_dat, write_addr, sizeof(OFFLINE_DAT));
	
	if (avaliable_count_64b <= total_count_64b) {
		avaliable_count_64b++;
	}

	write_pos_next_count_64b++;
	// OSMutexPost();
}

void offline_dump(OFFLINE_DAT *p_off_dat)
{
	u32 read_addr = 0;

	// OSMutexPend();
	if (total_count_64b == avaliable_count_64b) {
		read_pos_next_count_64b = write_pos_next_count_64b;
	}

	read_addr = OFFLINE_OFFSET + (read_pos_next_count_64b%total_count_64b)*OFFLINE_SIZE_PER;

	W25QXX_Read((u8*)&p_off_dat, read_addr, sizeof(OFFLINE_DAT));
	
	avaliable_count_64b--;
	read_pos_next_count_64b++;
	// OSMutexPost();
}

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
