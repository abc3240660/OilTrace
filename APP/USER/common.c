#include "common.h"
#include "malloc.h"
#include "os.h"
#include "rtc.h"
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

extern OS_MUTEX g_sem_w25q;

void parse_rtc_time(u8* buf);

void offline_init(void)
{
	total_count_64b = (W25Q_SETOR_COUNT-1)*W25Q_SECTOR_SIZE / OFFLINE_SIZE_PER;
	avaliable_count_64b = 0;
	read_pos_next_count_64b = 0;
	write_pos_next_count_64b = 0;
}

void get_mutex()
{
	OS_ERR err;
	OSMutexPend (  (OS_MUTEX    *)&g_sem_w25q,
					(OS_TICK    )0,
					(OS_OPT     )OS_OPT_PEND_BLOCKING,
					(CPU_TS     *)0,
					(OS_ERR     *)&err);
}

void free_mutex()
{
	OS_ERR err;
	OSMutexPost ( (OS_MUTEX  *)&g_sem_w25q,
        (OS_OPT   )OS_OPT_POST_NONE,
        (OS_ERR  *)&err);
}

void offline_write(float *val)
{
	u8 i = 0;
	u8 size = 0;
	u8 decimal_val = 0;
	u16 integer_val = 0;
	u32 write_addr = 0;
	
	OFFLINE_DAT off_dat;

	RTC_Get();

	size = sizeof(OFFLINE_DAT);

	parse_rtc_time(off_dat.systime);

	for (i=0; i<22; i++) {
		integer_val = (u16)val[i];
		decimal_val = (u16)((val[i]-integer_val)*100);

		off_dat.mes_val[i*2] = (u8)(integer_val>>1);
		off_dat.mes_val[i*2+1] = (u8)((integer_val&0x01)*128 + decimal_val);
	}
	
	get_mutex();
	write_addr = OFFLINE_OFFSET + (write_pos_next_count_64b%total_count_64b)*OFFLINE_SIZE_PER;
	
	W25QXX_Write((u8*)&off_dat, write_addr, sizeof(OFFLINE_DAT));
	
	if (avaliable_count_64b <= total_count_64b) {
		avaliable_count_64b++;
	}

	write_pos_next_count_64b++;
	free_mutex();
}

void offline_dump(OFFLINE_DAT *p_off_dat)
{
	u32 read_addr = 0;

	get_mutex();
	if (total_count_64b == avaliable_count_64b) {
		read_pos_next_count_64b = write_pos_next_count_64b;
	}

	read_addr = OFFLINE_OFFSET + (read_pos_next_count_64b%total_count_64b)*OFFLINE_SIZE_PER;

	W25QXX_Read((u8*)p_off_dat, read_addr, sizeof(OFFLINE_DAT));
	
	avaliable_count_64b--;
	read_pos_next_count_64b++;
	free_mutex();
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
