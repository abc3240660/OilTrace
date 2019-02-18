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

//START����
//������������ȼ�
#define START_TASK_PRIO				3
//�����ջ��С 
#define START_STK_SIZE			  256
//������ƿ�
OS_TCB StartTaskTCB;
//�����ջ	
CPU_STK START_TASK_STK[START_STK_SIZE];
//������
void start_task(void *p_arg);

//TEMP����
//�����������ȼ�
#define APPCMDS_TASK_PRIO 				5
//�����ջ��С
#define APPCMDS_STK_SIZE				64
//������ƿ�
OS_TCB AppCmdsTaskTCB;
//�����ջ
CPU_STK APPCMDS_TASK_STK[APPCMDS_STK_SIZE];
//led0����
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
	
	delay_init();	    	//��ʱ������ʼ��	  
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); 	//����NVIC�жϷ���2:2λ��ռ���ȼ���2λ��Ӧ���ȼ�

 	LED_Init();			    //LED�˿ڳ�ʼ��

	my_mem_init(SRAMIN); 	//��ʼ���ڲ��ڴ��
	
	//TIM3_Int_Init(9999, 7199);
	//uart_init(115200);	 	//���ڳ�ʼ��Ϊ115200
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
		
		if ((iapf0 == 0x3C3C) && (iapf1 == 0x4D4D)) {// BAK�������¹̼���־,��Ҫִ��BAK��APP�Ŀ���			
			for (i=0; i<100; i++) {// ������200K����
				for (j=0; j<1024; j++) {//u8���u16����ֽ�����					   
					IAPBUF[j] = *(u16*)(SYS_APP_SAVE_ADDR_BASE+k+i*2048); 	//����ʱFlash������ݶ�����	 ÿ�ζ�1K�ֽ�
					k += 2;
				}
				k = 0;
				FLASH_Unlock();
				FLASH_ClearFlag(FLASH_FLAG_EOP|FLASH_FLAG_PGERR|FLASH_FLAG_WRPRTERR);
				FLASH_ErasePage(SYS_APP_BAK_SAVE_ADDR_BASE+i*2048);// �Ȳ���д��ÿ�β���һҳ��ַ(1K);
				for (j=0; j<1024; j++) {// u8���u16����ֽ�����					   
					FLASH_ProgramHalfWord((SYS_APP_BAK_SAVE_ADDR_BASE+j*2+i*2048), IAPBUF[j]);// flashΪһ���ֽڴ洢��16λ���ݱ����ַ��2
				}
				FLASH_Lock();
				j = 0;
				delay_ms(2);
				// printf("���ڸ��¹̼� %d percent\r\n", (i*100)/224);
			}
			
			delay_ms(30);
			
			for (i=0; i<100; i++) {// ������100K����
				k = 0;
				for (j=0; j<1024; j++) {// u8���u16����ֽ�����					   
					iapf0 = *(u16*)(SYS_APP_SAVE_ADDR_BASE+k+i*2048);
					iapf1 = *(u16*)(SYS_APP_BAK_SAVE_ADDR_BASE+k+i*2048);
					
					if (iapf0 != iapf1) {
						g_ota_bin_size = 0;
					}
					k += 2;
				}
			}
			
			// printf("У�����!\r\n");
			UP_success();// ���±�־,�´�ֱ�Ӵ�APP��������
			
			if (((*(vu32*)(SYS_APP_BAK_SAVE_ADDR_BASE+4))&0xFF000000) == 0x08000000) {// �ж��Ƿ�Ϊ0X08XXXXXX.
				iap_load_app(SYS_APP_BAK_SAVE_ADDR_BASE);// ִ�и��º��APP����
			}
		} else {// ����ִ����ת��������APP����

			for (i=0; i<100; i++) {// ������100K����
				k = 0;
				for (j=0; j<1024; j++) {// u8���u16����ֽ�����					   
					iapf0 = *(u16*)(SYS_APP_SAVE_ADDR_BASE+k+i*2048);
					iapf1 = *(u16*)(SYS_APP_BAK_SAVE_ADDR_BASE+k+i*2048);
					
					if (iapf0 != iapf1) {
						g_ota_bin_size = 0;
					}
					k += 2;
				}
			}
			
			if (((*(vu32*)(SYS_APP_SAVE_ADDR_BASE+4))&0xFF000000) == 0x08000000) {// �ж��Ƿ�Ϊ0X08XXXXXX.
				iap_load_app(SYS_APP_SAVE_ADDR_BASE);// ִ�г���ʱ��APP����
			}
		}
	}

	// KEY PUSHED Before Boot
	// Enter into IAP Mode
	// Get Opportunity to Receive UART MSG below here

	OSInit(&err);		//��ʼ��UCOSIII
	OS_CRITICAL_ENTER();//�����ٽ���
	//������ʼ����
	OSTaskCreate((OS_TCB 	* )&StartTaskTCB,		//������ƿ�
				 (CPU_CHAR	* )"start task", 		//��������
                 (OS_TASK_PTR )start_task, 			//������
                 (void		* )0,					//���ݸ��������Ĳ���
                 (OS_PRIO	  )START_TASK_PRIO,     //�������ȼ�
                 (CPU_STK   * )&START_TASK_STK[0],	//�����ջ����ַ
                 (CPU_STK_SIZE)START_STK_SIZE/10,	//�����ջ�����λ
                 (CPU_STK_SIZE)START_STK_SIZE,		//�����ջ��С
                 (OS_MSG_QTY  )0,					//�����ڲ���Ϣ�����ܹ����յ������Ϣ��Ŀ,Ϊ0ʱ��ֹ������Ϣ
                 (OS_TICK	  )0,					//��ʹ��ʱ��Ƭ��תʱ��ʱ��Ƭ���ȣ�Ϊ0ʱΪĬ�ϳ��ȣ�
                 (void   	* )0,					//�û�����Ĵ洢��
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR, //����ѡ��
                 (OS_ERR 	* )&err);				//��Ÿú�������ʱ�ķ���ֵ
	OS_CRITICAL_EXIT();	//�˳��ٽ���	 
	OSStart(&err);  //����UCOSIII
}

//��ʼ������
void start_task(void *p_arg)
{
	OS_ERR err;
	CPU_SR_ALLOC();
	p_arg = p_arg;

	CPU_Init();
#if OS_CFG_STAT_TASK_EN > 0u
   OSStatTaskCPUUsageInit(&err);  	//ͳ������                
#endif
	
#ifdef CPU_CFG_INT_DIS_MEAS_EN		//���ʹ���˲����жϹر�ʱ��
    CPU_IntDisMeasMaxCurReset();	
#endif

#if	OS_CFG_SCHED_ROUND_ROBIN_EN  //��ʹ��ʱ��Ƭ��ת��ʱ��
	 //ʹ��ʱ��Ƭ��ת���ȹ���,ʱ��Ƭ����Ϊ1��ϵͳʱ�ӽ��ģ���1*5=5ms
	OSSchedRoundRobinCfg(DEF_ENABLED,1,&err);  
#endif		
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_CRC,ENABLE);//����CRCʱ��

	OS_CRITICAL_ENTER();	//�����ٽ���

#if 1
	// �¶ȶ�ȡ����
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
	// IAP����
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
								 
	OS_TaskSuspend((OS_TCB*)&StartTaskTCB,&err);		//����ʼ����			 
	OS_CRITICAL_EXIT();	//�˳��ٽ���
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
// ÿ��2�����һ���¶���Ϣ
// �ú����е��¶���ϢӦȫ����Ϊ���¶Ȱ���ȡ
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
		
		OSTimeDlyHMSM(0,0,0,100,OS_OPT_TIME_PERIODIC,&err);//��ʱ500ms
	}
}
