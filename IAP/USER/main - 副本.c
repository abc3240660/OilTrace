#include "led.h"
#include "delay.h"
#include "sys.h"
#include "usart.h"
#include "timer.h"
#include "malloc.h"
#include "includes.h"
#include "common.h"
#include "dma.h"
#include "iap.h"
#include "gagent_md5.h"
#include "stmflash.h"

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
#define APPCMDS_STK_SIZE				64
//任务控制块
OS_TCB AppCmdsTaskTCB;
//任务堆栈
CPU_STK APPCMDS_TASK_STK[APPCMDS_STK_SIZE];
//led0任务
void app_cmds_task(void *p_arg);

#define IAP_TASK_PRIO 				8
#define IAP_STK_SIZE					1024
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

u8 g_rx_sta_clr_pos = 0;

u8 g_ota_bin_md5[SSL_MAX_LEN] = {0};
u8 g_ota_package_md5[SSL_MAX_LEN] = {0};

int main(void)
{
	OS_ERR err;
	CPU_SR_ALLOC();	
	
	delay_init();	    	//延时函数初始化	  
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); 	//设置NVIC中断分组2:2位抢占优先级，2位响应优先级

 	LED_Init();			    //LED端口初始化

	my_mem_init(SRAMIN); 	//初始化内部内存池
	
	//TIM3_Int_Init(9999, 7199);
	//uart_init(115200);	 	//串口初始化为115200
	//Usart_DMA_Init();
	
	// printf("Welcom SomkeGrill\n");

	// Normal Boot Mode
	if (1) {
		u16 i;
		u32 j,k;
		u16 iapf0,iapf1;
		u16	IAPBUF[1024];
		
		iapf0 = *(u16*)(UPDATE_PARAM_SAVE_ADDR_BASE);
		iapf1 = *(u16*)(UPDATE_PARAM_SAVE_ADDR_BASE+2);
		
		if ((iapf0 == 0x3C3C) && (iapf1 == 0x4D4D)) {// BAK分区有新固件标志,需要执行BAK到APP的拷贝			
			for (i=0; i<100; i++) {// 不超过200K程序
				for (j=0; j<1024; j++) {//u8变成u16后的字节数据					   
					IAPBUF[j] = *(u16*)(SYS_APP_SAVE_ADDR_BASE+k+i*2048); 	//将临时Flash里的数据读出来	 每次读1K字节
					k += 2;
				}
				k = 0;
				FLASH_Unlock();
				FLASH_ClearFlag(FLASH_FLAG_EOP|FLASH_FLAG_PGERR|FLASH_FLAG_WRPRTERR);
				FLASH_ErasePage(SYS_APP_BAK_SAVE_ADDR_BASE+i*2048);// 先擦后写，每次擦除一页地址(1K);
				for (j=0; j<1024; j++) {// u8变成u16后的字节数据					   
					FLASH_ProgramHalfWord((SYS_APP_BAK_SAVE_ADDR_BASE+j*2+i*2048), IAPBUF[j]);// flash为一个字节存储，16位数据必须地址加2
				}
				FLASH_Lock();
				j = 0;
				delay_ms(2);
				// printf("正在更新固件 %d percent\r\n", (i*100)/224);
			}
			
			delay_ms(30);
			
			for (i=0; i<100; i++) {// 不超过100K程序
				k = 0;
				for (j=0; j<1024; j++) {// u8变成u16后的字节数据					   
					iapf0 = *(u16*)(SYS_APP_SAVE_ADDR_BASE+k+i*2048);
					iapf1 = *(u16*)(SYS_APP_BAK_SAVE_ADDR_BASE+k+i*2048);
					
					if (iapf0 != iapf1) {
						g_ota_bin_size = 0;
					}
					k += 2;
				}
			}
			
			// printf("校验完毕!\r\n");
			UP_success();// 更新标志,下次直接从APP分区启动
			
			if (((*(vu32*)(SYS_APP_BAK_SAVE_ADDR_BASE+4))&0xFF000000) == 0x08000000) {// 判断是否为0X08XXXXXX.
				iap_load_app(SYS_APP_BAK_SAVE_ADDR_BASE);// 执行更新后的APP代码
			}
		} else {// 否则执行跳转到出厂的APP程序

			for (i=0; i<100; i++) {// 不超过100K程序
				k = 0;
				for (j=0; j<1024; j++) {// u8变成u16后的字节数据					   
					iapf0 = *(u16*)(SYS_APP_SAVE_ADDR_BASE+k+i*2048);
					iapf1 = *(u16*)(SYS_APP_BAK_SAVE_ADDR_BASE+k+i*2048);
					
					if (iapf0 != iapf1) {
						g_ota_bin_size = 0;
					}
					k += 2;
				}
			}
			
			if (((*(vu32*)(SYS_APP_SAVE_ADDR_BASE+4))&0xFF000000) == 0x08000000) {// 判断是否为0X08XXXXXX.
				iap_load_app(SYS_APP_SAVE_ADDR_BASE);// 执行出厂时的APP代码
			}
		}
	}

	// KEY PUSHED Before Boot
	// Enter into IAP Mode
	// Get Opportunity to Receive UART MSG below here

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
void iap_task(void *p_arg)
{
	OS_ERR err;
	
	u8 i = 0;
	u8 md5_pass = 1;
	u32 USART_RX_STA_BAK = 0;
	u8 bin_md5_calc[SSL_MAX_LEN];

	MD5_CTX g_ota_md5_ctx;
	
	while (1) {
		if (0x80000 == (USART_RX_STA&(1<<19))) {
			if ((0xFE != USART_RX_BUF[0]) || (0xFF != USART_RX_BUF[((USART_RX_STA&0xFFFF) - 1)])) {
				// To Accelerate the usage of USART_RX_BUF
				USART_RX_STA_BAK = USART_RX_STA;
				memcpy(USART_RX_BUF_BAK, USART_RX_BUF, USART_RX_STA&0xFFFF);

				// Go On to Receive ASAP
				USART_RX_STA = 0;
				g_rx_sta_clr_pos = 1;
				DMA_SetCurrDataCounter(DMA1_Channel5, U1_DMA_R_LEN);
				DMA_Cmd(DMA1_Channel5, ENABLE);

				if (1 == g_ota_pg_numid) {
					// MD5 for total bin
					GAgent_MD5Init(&g_ota_md5_ctx);
					memset(bin_md5_calc, 0, SSL_MAX_LEN);
				}

				do {
					// MD5 for single package
					MD5_CTX package_md5_ctx;
					u8 package_md5_calc[SSL_MAX_LEN] = {0};
					
					if (0 == g_ota_sta) {
						// printf("OTA Stop By APP!!!\n");
						break;
					}
					if ((g_ota_recv_sum+USART_RX_STA_BAK&0xFFFF) > g_ota_bin_size) {
						// printf("OTA TotalSize Overflow!!!\n");
						// g_ota_sta = 2;
						UART1_ReportOtaPackageSta(0);
						break;
					}

					if (g_ota_pg_numid > g_ota_pg_nums) {
						// printf("OTA PGNUM Overflow!!!\n");
						// g_ota_sta = 2;
						UART1_ReportOtaPackageSta(0);
						break;
					}

					GAgent_MD5Init(&package_md5_ctx);
					GAgent_MD5Update(&package_md5_ctx, USART_RX_BUF_BAK, (USART_RX_STA_BAK&0xFFFF));
          GAgent_MD5Final(&package_md5_ctx, package_md5_calc);

					if (memcmp(package_md5_calc, g_ota_package_md5, SSL_MAX_LEN) != 0) {
						md5_pass = 0;
						UART1_ReportOtaPackageSta(0);
					} else {
						md5_pass = 1;
						UART1_ReportOtaPackageSta(1);
					}

					// MD5 for total bin
					GAgent_MD5Update(&g_ota_md5_ctx, USART_RX_BUF_BAK, (USART_RX_STA_BAK&0xFFFF));

					if (md5_pass) {
						// printf("Before Write: Addr(0x%.6X), SingleSize(%d)\n", FLASH_BAK_ADDR+g_ota_recv_sum, (USART_RX_STA_BAK&0xFFFF));
						iap_write_appbin(FLASH_BAK_ADDR+g_ota_recv_sum, USART_RX_BUF_BAK, (USART_RX_STA_BAK&0xFFFF));
						g_ota_recv_sum += USART_RX_STA_BAK&0xFFFF;
						// printf("After Write: IAP Packet(%d), TotalSize(0x%.6X)\n", g_ota_pg_numid, g_ota_recv_sum);
					}
				} while (0);

				//if (2 == g_ota_sta) {
				//	UART1_ReportOtaPackageSta(0);
					// GUIDEMO_IapWarnIcon(2);// Fail
				//}

				if (g_ota_pg_nums == g_ota_pg_numid) {
					if (g_ota_recv_sum != g_ota_bin_size) {
						// printf("OTA TotalSize Error!!!\n");
						g_ota_sta = 2;
						UART1_ReportOtaBinSta(2);// Fail
					} else {
						// MD5 for total bin
						GAgent_MD5Final(&g_ota_md5_ctx, bin_md5_calc);

						if (memcmp(bin_md5_calc, g_ota_bin_md5, SSL_MAX_LEN) != 0) {
							md5_pass = 0;
							g_ota_sta = 2;
						} else {
							md5_pass = 1;
						}

						if (md5_pass) {
							// printf("OTA TotalWrite Success!!!\n");

							g_ota_sta = 1;
							UART1_ReportOtaBinSta(1);// Pass
							// Mark Flag in Flash
							download_success();
							SoftReset();
						}
					}
				}
			}
		}
		
		if (30 == i++) {
			i = 0;
			if (g_printf_enable != 0) {
				printf("APP1 STA = %X, g_rx_sta_clr_pos=%d\n", USART_RX_STA, g_rx_sta_clr_pos);
			}
		}
		
		OSTimeDlyHMSM(0,0,0,100,OS_OPT_TIME_PERIODIC,&err);
	}
}

void process_app_cmds(void)
{
	if (0x80000 == (USART_RX_STA&(1<<19))) {
		if ((0xFE == USART_RX_BUF[0]) && (0xFF == USART_RX_BUF[((USART_RX_STA&0xFFFF) - 1)])) {
			run_cmd_from_usart(USART_RX_BUF, (USART_RX_STA&0xFFFF));

			USART_RX_STA = 0;
			g_rx_sta_clr_pos = 2;
			DMA_SetCurrDataCounter(DMA1_Channel5, U1_DMA_R_LEN);
			DMA_Cmd(DMA1_Channel5, ENABLE);
		}
	}
}
// 每隔2秒更新一次温度信息
// 该函数中的温度信息应全部改为从温度棒读取
void app_cmds_task(void *p_arg)
{
	u8 i = 0;
	OS_ERR err;
		
	while(1)
	{
		LED0 = !LED0;
		
		process_app_cmds();

		if (50 == i++) {
			if (0 == g_ota_sta) {
				UART1_ButtonOtaReport();
			}
			i = 0;
		}
		
		OSTimeDlyHMSM(0,0,0,100,OS_OPT_TIME_PERIODIC,&err);//延时500ms
	}
}
