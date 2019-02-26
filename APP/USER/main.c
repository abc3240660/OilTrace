#include "led.h"
#include "delay.h"
#include "sys.h"
#include "usart.h"
#include "timer.h"
#include "malloc.h"
#include "includes.h"
#include "common.h"
#include "dma.h"
#include "gagent_md5.h"
#include "iap.h"
#include "w25qxx.h"
#include "ad.h"
#include "rtc.h"

#define PKT_HEAD_SIZE 3
#define PKT_DEVID_SIZE 12
#define PKT_CMD_SIZE 1
#define PKT_CRC_SIZE 16
#define PKT_TAIL_SIZE 3

#pragma pack(1)
typedef struct protocol{
	u8 pro_head[PKT_HEAD_SIZE+1];
	u8 dev_id[PKT_DEVID_SIZE+1];
	u8 pro_cmd;
	u8 *p_data;
	u8 pro_crc[PKT_CRC_SIZE+1];
	u8 pro_tail[PKT_TAIL_SIZE+1];
	u32 data_len;
} OIL_PRO;
#pragma pack()

//START任务
//设置任务的优先级
#define START_TASK_PRIO				3
//任务堆栈大小 
#define START_STK_SIZE			  256
//任务控制块
OS_TCB StartTaskTCB;
//任务堆栈	
CPU_STK START_TASK_STK[START_STK_SIZE];
//任务函数
void start_task(void *p_arg);

//TEMP任务
//设置任务优先级
#define APPCMDS_TASK_PRIO 				5
//任务堆栈大小
#define APPCMDS_STK_SIZE				512
//任务控制块
OS_TCB AppCmdsTaskTCB;
//任务堆栈
CPU_STK APPCMDS_TASK_STK[APPCMDS_STK_SIZE];
//led0任务
void app_cmds_task(void *p_arg);

#define IAP_TASK_PRIO 				8
#define IAP_STK_SIZE					512
OS_TCB IapTaskTCB;
CPU_STK IAP_TASK_STK[IAP_STK_SIZE];
void iap_task(void *p_arg);
void UART1_ButtonOtaReport(void);
	
//////////////////////////////////////////////////////////////////////////////

u8 g_printf_enable = 0;
extern void UART1_ReportOtaPackageSta(u8 md5_res);
extern void UART1_ReportOtaBinSta(u8 md5_res);
// 0-IDEL, 1-pass, 2-fail, 3-in progress
u8 g_ota_sta = 0;
u32 g_ota_recv_sum = 0;
u16 g_ota_pg_numid = 0;
u16 g_ota_pg_nums = 0;
u32 g_ota_bin_size = 0;

u8 g_ota_bin_md5[SSL_MAX_LEN] = {0};
u8 g_ota_package_md5[SSL_MAX_LEN] = {0};

OIL_PRO recv_pack;

u32 USART_RX_STA_BAK = 0;

u16 ad_val[22] = {0};
float	ad_val_f[22] = {0};

float	measured_val[22];

extern u32 os_jiffies;

u16 report_gap = 0;
u16 hbeat_gap = 0;

u8 net_ok = 0;
u16 hbeat_cnt = 0;

u32 time_start_hbeat = 0;
u32 time_start_report = 0;
u32 time_start_srv_hbeat = 0;

extern SYS_ENV g_sys_env_old;
extern SYS_ENV g_sys_env_new;

// 0-not configed
// 1-from spi flash
// 2-from server
u8 sys_env_mode = 0;

int main(void)
{	
	OS_ERR err;
	CPU_SR_ALLOC();	
	
	delay_init();	    	//延时函数初始化	  
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); 	//设置NVIC中断分组2:2位抢占优先级，2位响应优先级

 	LED_Init();			    //LED端口初始化

	my_mem_init(SRAMIN); 	//初始化内部内存池
	
	TIM3_Int_Init(999, 7199);
	uart_init(115200);	 	//串口初始化为115200
	//uart2_init(115200);
	//Usart_DMA_Init();

//	while(1) {
//		printf("Welcom to OilTrace!!!\n");
//		delay_ms(1000);
//	}
	
	RTC_Init();
	RTC_Get();

	W25QXX_Init();
	
	sys_env_dump();

	// SPI_MCP3204_Init();
	AD3204_Init();

	// UART1_HeartBeat();
	UART1_ParamsRequest();
	time_start_srv_hbeat = os_jiffies;
	//UART1_HeartBeat();
	//UART1_ReportTestSta();
	
	OSInit(&err);		//初始化UCOSIII
	OS_CRITICAL_ENTER();//进入临界区
	//创建开始任务
	OSTaskCreate((OS_TCB 	* )&StartTaskTCB,		//任务控制块
				 (CPU_CHAR	* )"start task", 		//任务名字
                 (OS_TASK_PTR )start_task, 			//任务函数
                 (void		* )0,					//传递给任务函数的参数
                 (OS_PRIO	  )START_TASK_PRIO,     //任务优先级
                 (CPU_STK   * )&START_TASK_STK[0],	//任务堆栈基地址
                 (CPU_STK_SIZE)START_STK_SIZE/10,	//任务堆栈深度限位
                 (CPU_STK_SIZE)START_STK_SIZE,		//任务堆栈大小
                 (OS_MSG_QTY  )0,					//任务内部消息队列能够接收的最大消息数目,为0时禁止接收消息
                 (OS_TICK	  )0,					//当使能时间片轮转时的时间片长度，为0时为默认长度，
                 (void   	* )0,					//用户补充的存储区
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR, //任务选项
                 (OS_ERR 	* )&err);				//存放该函数错误时的返回值
	OS_CRITICAL_EXIT();	//退出临界区	 
	OSStart(&err);  //开启UCOSIII
}

//开始任务函数
void start_task(void *p_arg)
{
	OS_ERR err;
	CPU_SR_ALLOC();
	p_arg = p_arg;

	CPU_Init();
#if OS_CFG_STAT_TASK_EN > 0u
   OSStatTaskCPUUsageInit(&err);  	//统计任务                
#endif
	
#ifdef CPU_CFG_INT_DIS_MEAS_EN		//如果使能了测量中断关闭时间
    CPU_IntDisMeasMaxCurReset();	
#endif

#if	OS_CFG_SCHED_ROUND_ROBIN_EN  //当使用时间片轮转的时候
	 //使能时间片轮转调度功能,时间片长度为1个系统时钟节拍，既1*5=5ms
	OSSchedRoundRobinCfg(DEF_ENABLED,1,&err);  
#endif		
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_CRC,ENABLE);//开启CRC时钟

	OS_CRITICAL_ENTER();	//进入临界区

#if 1
	// 温度读取任务
	OSTaskCreate((OS_TCB*     )&AppCmdsTaskTCB,		
				 (CPU_CHAR*   )"Temp task", 		
                 (OS_TASK_PTR )app_cmds_task, 			
                 (void*       )0,					
                 (OS_PRIO	  )APPCMDS_TASK_PRIO,     
                 (CPU_STK*    )&APPCMDS_TASK_STK[0],	
                 (CPU_STK_SIZE)APPCMDS_STK_SIZE/10,	
                 (CPU_STK_SIZE)APPCMDS_STK_SIZE,		
                 (OS_MSG_QTY  )0,					
                 (OS_TICK	  )0,  					
                 (void*       )0,					
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR,
                 (OS_ERR*     )&err);
#endif					 
#if 1
	// IAP任务
	OSTaskCreate((OS_TCB*     )&IapTaskTCB,		
				 (CPU_CHAR*   )"IAP task", 		
                 (OS_TASK_PTR )iap_task, 			
                 (void*       )0,					
                 (OS_PRIO	  )IAP_TASK_PRIO,     
                 (CPU_STK*    )&IAP_TASK_STK[0],	
                 (CPU_STK_SIZE)IAP_STK_SIZE/10,	
                 (CPU_STK_SIZE)IAP_STK_SIZE,		
                 (OS_MSG_QTY  )0,					
                 (OS_TICK	  )0,  					
                 (void*       )0,					
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR,
                 (OS_ERR*     )&err);
#endif								 
								 
	OS_TaskSuspend((OS_TCB*)&StartTaskTCB,&err);		//挂起开始任务			 
	OS_CRITICAL_EXIT();	//退出临界区
}

void SoftReset(void)
{  
	__set_FAULTMASK(1);
 	NVIC_SystemReset();
}

extern u32 USART_RX_STA;
extern u32 USART_RX_STA2;
void iap_task(void *p_arg)
{
	OS_ERR err;
	
	u8 i = 0;
	
	while (1) {
		if (0x80000 == (USART_RX_STA_BAK&(1<<19))) {
			//g_ota_pg_numid = USART_RX_BUF_BAK[0];
			//USART_RX_STA_BAK -= 1;

			do {
				if ((g_ota_recv_sum+USART_RX_STA_BAK&0xFFFF) > g_ota_bin_size) {
					// u1_printf("OTA TotalSize Overflow!!!\n");
					UART1_ReportOtaPackageSta(0);
					break;
				}

				UART1_ReportOtaPackageSta(0);// Pass

				// u1_printf("Before Write: Addr(0x%.6X), SingleSize(%d)\n", FLASH_BAK_ADDR+g_ota_recv_sum, (USART_RX_STA_BAK&0xFFFF));
				iap_write_appbin(FLASH_BAK_ADDR+g_ota_recv_sum, USART_RX_BUF_BAK, USART_RX_STA_BAK&0xFFFF);
				g_ota_recv_sum += USART_RX_STA_BAK&0xFFFF;
				// u1_printf("After Write: IAP Packet(%d), TotalSize(0x%.6X)\n", g_ota_pg_numid, g_ota_recv_sum);
			} while (0);
			
			USART_RX_STA_BAK = 0;

			if (g_ota_recv_sum == g_ota_bin_size) {
				UART1_ReportOtaBinSta(0);// Pass
				// Mark Flag in Flash
				download_success();
				SoftReset();
			}
		}
		
		if (30 == i++) {
			i = 0;
			if (g_printf_enable != 0) {
				u1_printf("APP1 STA = %X\n", USART_RX_STA2);
			}
		}
		
		OSTimeDlyHMSM(0,0,0,100,OS_OPT_TIME_PERIODIC,&err);
	}
}

void parse_server_params(u8* data, u16 size)
{
	u8 j = 0;
	u16 i = 0;

	if (209 == size) {
		_calendar_obj svr_time;
		
		net_ok = 1;
		time_start_hbeat = os_jiffies;
		time_start_report = os_jiffies;
		
		memset(&svr_time, 0, sizeof(_calendar_obj));
		
		svr_time.w_year  = (data[0]>>4)*1000 + (data[0]&0xF)*100 + (data[1]>>4)*10 + (data[1]&0xF);
		svr_time.w_month = (data[2]>>4)*10 + (data[2]&0xF);
		svr_time.w_date  = (data[3]>>4)*10 + (data[3]&0xF);

		svr_time.hour  = (data[4]>>4)*10 + (data[4]&0xF);
		svr_time.min   = (data[5]>>4)*10 + (data[5]&0xF);
		svr_time.sec   = (data[6]>>4)*10 + (data[6]&0xF);
		
		RTC_Set(svr_time.w_year,svr_time.w_month,svr_time.w_date,svr_time.hour,svr_time.min,svr_time.sec);  //设置时间

		report_gap = (data[7+1]<<8) + data[7+0];
		hbeat_gap  = (data[7+3]<<8) + data[7+2];
		
		if (report_gap < 250) {
			report_gap = 250;// 500ms
		}

		if (hbeat_gap < 1) {
			hbeat_gap = 1;// 1s
		}
		
		for (i=0; i<22; i++) {
			g_sys_env_new.ran_max[j] = (data[11+9*i+0]*2 + (data[11+9*i+1]>>7)) + (float)(data[11+9*i+1]&0x7F)/100;
			g_sys_env_new.ran_min[j] = (data[11+9*i+2]*2 + (data[11+9*i+3]>>7)) + (float)(data[11+9*i+3]&0x7F)/100;
			g_sys_env_new.war_max[j] = (data[11+9*i+4]*2 + (data[11+9*i+5]>>7)) + (float)(data[11+9*i+5]&0x7F)/100;
			g_sys_env_new.war_min[j] = (data[11+9*i+6]*2 + (data[11+9*i+7]>>7)) + (float)(data[11+9*i+7]&0x7F)/100;
			g_sys_env_new.war_mode[j] = data[11+9*i+7];
			j++;
		}

		sys_env_update();
	}
}

void parse_oil_pro(OIL_PRO *p_oil_pro)
{
	u16 dat_len = 0;
	char* str = NULL;
	MD5_CTX package_md5_ctx;
	u8 package_md5_calc[SSL_MAX_LEN] = {0};
					
	if ((str = strstr((const char *)USART_RX_BUF2, "STA"))) {
		if ((str = strstr((const char *)USART_RX_BUF2+((USART_RX_STA2&0xFFFF)-3), "END"))) {
			u1_printf("package format is OK\n");
		} else {
			u1_printf("package format tail is NG\n");
			return;
		}
	} else {
		u1_printf("package format head is NG\n");
		return;
	}
	
	if ((USART_RX_STA2&0xFFFF) < (PKT_HEAD_SIZE+PKT_DEVID_SIZE+PKT_CMD_SIZE+PKT_CRC_SIZE+PKT_TAIL_SIZE)) {
		u1_printf("package format size is NG\n");
		return;
	}

	memcpy(p_oil_pro->pro_crc, (char*)USART_RX_BUF2+((USART_RX_STA2&0xFFFF) - (PKT_CRC_SIZE+PKT_TAIL_SIZE)), PKT_CRC_SIZE);
	u1_printf("pro_crc = %s\n", p_oil_pro->pro_crc);

	dat_len = ((USART_RX_STA2&0xFFFF) - (PKT_HEAD_SIZE+PKT_CRC_SIZE+PKT_TAIL_SIZE));
	GAgent_MD5Init(&package_md5_ctx);
	GAgent_MD5Update(&package_md5_ctx, USART_RX_BUF2+PKT_HEAD_SIZE, dat_len);
  GAgent_MD5Final(&package_md5_ctx, package_md5_calc);

	if (memcmp(package_md5_calc, p_oil_pro->pro_crc, SSL_MAX_LEN) != 0) {
		return;// TBD Enable
	}

	memcpy(p_oil_pro->pro_head, (char*)USART_RX_BUF2, PKT_HEAD_SIZE);
	u1_printf("pro_head = %s\n", p_oil_pro->pro_head);
	
	memcpy(p_oil_pro->dev_id, (char*)USART_RX_BUF2+PKT_HEAD_SIZE, PKT_DEVID_SIZE);
	u1_printf("dev_id = %s\n", p_oil_pro->dev_id);
	
	memcpy(&p_oil_pro->pro_cmd, (char*)USART_RX_BUF2+PKT_HEAD_SIZE+PKT_DEVID_SIZE, PKT_CMD_SIZE);
	u1_printf("pro_cmd = %s\n", p_oil_pro->pro_cmd);

	p_oil_pro->data_len = (USART_RX_STA2&0xFFFF) - (PKT_HEAD_SIZE+PKT_DEVID_SIZE+PKT_CMD_SIZE+PKT_CRC_SIZE+PKT_TAIL_SIZE);

	if (0x02 == p_oil_pro->pro_cmd) {
		if (p_oil_pro->data_len != 3) {// Max 3Bytes
			return;
		} else {
			u8 *p_size_data = USART_RX_BUF2+PKT_HEAD_SIZE+PKT_DEVID_SIZE+PKT_CMD_SIZE;
			g_ota_bin_size = ((*(p_size_data))<<16) + ((*(p_size_data+1))<<8) + *(p_size_data+2);
			
			g_ota_recv_sum = 0;
		}
	} else if (0x03 == p_oil_pro->pro_cmd) {
		if (p_oil_pro->data_len > 2049) {// Max: 1B PackNum + 2048B Data
			return;
		}
		if (p_oil_pro->data_len != 0) {
			while (USART_RX_STA_BAK);
			
			memcpy(USART_RX_BUF_BAK, (char*)USART_RX_BUF2+PKT_HEAD_SIZE+PKT_DEVID_SIZE+PKT_CMD_SIZE, p_oil_pro->data_len);
			p_oil_pro->p_data = (u8*)USART_RX_BUF_BAK;
			USART_RX_STA_BAK = p_oil_pro->data_len;
			USART_RX_STA_BAK |= 1<<19;
		}
	} else if (0x06 == p_oil_pro->pro_cmd) {
		parse_server_params((u8*)USART_RX_BUF2+PKT_HEAD_SIZE+PKT_DEVID_SIZE+PKT_CMD_SIZE, p_oil_pro->data_len);
	} else if (0x07 == p_oil_pro->pro_cmd) {
		// heart beat
		hbeat_cnt = 0;
		time_start_srv_hbeat = os_jiffies;
	}
	
	memcpy(p_oil_pro->pro_tail, (char*)USART_RX_BUF2+((USART_RX_STA2&0xFFFF) - PKT_TAIL_SIZE), PKT_TAIL_SIZE);
	u1_printf("pro_tail = %s\n", p_oil_pro->pro_tail);
}

void process_app_cmds(void)
{
	if (0x80000 == (USART_RX_STA2&(1<<19))) {
		if (('S' == USART_RX_BUF2[0]) && ('D' == USART_RX_BUF2[((USART_RX_STA2&0xFFFF) - 1)])) {
			memset(&recv_pack, 0, sizeof(OIL_PRO));
			parse_oil_pro(&recv_pack);

			USART_RX_STA2 = 0;
			// DMA_SetCurrDataCounter(DMA1_Channel5, U1_DMA_R_LEN);
			// DMA_Cmd(DMA1_Channel5, ENABLE);
		}
	}
}

// timeout unit: ms
// timeout = 100ms * N (N >= 1)
u8 is_timeout(u32 time_start, u32 timeout)
{
	if (os_jiffies > time_start) {
		if ((os_jiffies-time_start) > (timeout / 100)) {
			return 1;
		}
	} else {
		if ((10000-time_start+os_jiffies) > (timeout / 100)) {
			return 1;
		}
	}
	
	return 0;
}

// 每隔2秒更新一次温度信息
// 该函数中的温度信息应全部改为从温度棒读取
void app_cmds_task(void *p_arg)
{
	u8 i = 0;
	u8 j = 0;
	u16 w25q_type = 0;
	
	u8 triger_hbeat = 0;
	u8 triger_report = 0;
	u8 scan_all = 0;
	
	OS_ERR err;

	w25q_type = W25QXX_ReadID();
	
	while(1)
	{
		LED0 = !LED0;
		
		process_app_cmds();

		if (1 == net_ok) {
			if (is_timeout(time_start_hbeat, hbeat_gap*1000)) {
				triger_hbeat = 1;
				time_start_hbeat = os_jiffies;
			} else {
				// send offline warning to server
				// UART1_AdValReportOffline(ad_val);
			}

			if (is_timeout(time_start_report, report_gap)) {
				triger_report = 1;
				time_start_report = os_jiffies;
			}
		} else {
			UART1_ParamsRequest();
		}

		if (1 == triger_report) {
			if (1 == scan_all) {
				triger_report = 0;

				for (j=0; j<22; j++) {
					if (1 == sys_env_mode) {
						measured_val[j] = (ad_val_f[j]*(g_sys_env_old.ran_max[j]-g_sys_env_old.ran_min[j])/20000)+g_sys_env_old.ran_min[j];
					} else if (2 == sys_env_mode) {
						measured_val[j] = (ad_val_f[j]*(g_sys_env_new.ran_max[j]-g_sys_env_new.ran_min[j])/20000)+g_sys_env_new.ran_min[j];
					}
				}
				
				UART1_AdValReport(measured_val);
			}
		}
		
		if (1 == triger_hbeat) {
			triger_hbeat = 0;
			UART1_HeartBeat();
		}

		if (i >= 22) {
			i = 0;
			scan_all = 1;
		}

		ad_val[i] = (SPI_Read(i)) & 0xFFF;
		ad_val_f[i] = ((float)ad_val[i]/4096)*2.5*10000;

		i++;
		
		if (hbeat_cnt >= 5000) {// 50s
			net_ok = 0;
			// save offline warning&time into flash
		} else {
			hbeat_cnt++;
		}

		OSTimeDlyHMSM(0,0,0,10,OS_OPT_TIME_PERIODIC,&err);//延时10ms
	}
}
