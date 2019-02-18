#include "sys.h"
#include "usart.h"	 
#include "dma.h"
#include "common.h"
#include "timer.h"
#include "malloc.h"
#include "gagent_md5.h"
////////////////////////////////////////////////////////////////////////////////// 	 
//���ʹ��os,����������ͷ�ļ�����.
#if SYSTEM_SUPPORT_OS
#include "includes.h"					//os ʹ��	  
#endif
//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//ALIENTEK STM32������
//����1��ʼ��		   
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//�޸�����:2012/8/18
//�汾��V1.5
//��Ȩ���У�����ؾ���
//Copyright(C) ������������ӿƼ����޹�˾ 2009-2019
//All rights reserved
//********************************************************************************
//V1.3�޸�˵�� 
//֧����Ӧ��ͬƵ���µĴ��ڲ���������.
//�����˶�printf��֧��
//�����˴��ڽ��������.
//������printf��һ���ַ���ʧ��bug
//V1.4�޸�˵��
//1,�޸Ĵ��ڳ�ʼ��IO��bug
//2,�޸���USART_RX_STA,ʹ�ô����������ֽ���Ϊ2��14�η�
//3,������USART_MAX_RECV_LEN,���ڶ��崮�����������յ��ֽ���(������2��14�η�)
//4,�޸���EN_USART1_RX��ʹ�ܷ�ʽ
//V1.5�޸�˵��
//1,�����˶�UCOSII��֧��
////////////////////////////////////////////////////////////////////////////////// 	  
 

//////////////////////////////////////////////////////////////////
//�������´���,֧��printf����,������Ҫѡ��use MicroLIB	  
#if 1
#pragma import(__use_no_semihosting)             
//��׼����Ҫ��֧�ֺ���                 
struct __FILE 
{ 
	int handle; 

}; 

FILE __stdout;       
//����_sys_exit()�Ա���ʹ�ð�����ģʽ    
_sys_exit(int x) 
{ 
	x = x; 
} 
//�ض���fputc���� 
int fputc(int ch, FILE *f)
{      
	while((USART1->SR&0X40)==0);//ѭ������,ֱ���������   
    USART1->DR = (u8) ch;      
	return ch;
}
#endif 
 
#if EN_USART1_RX   //���ʹ���˽���
//����1�жϷ������
//ע��,��ȡUSARTx->SR�ܱ���Ī������Ĵ���   	
u8* USART_RX_BUF = NULL;
u8* USART_RX_BUF_BAK = NULL;
u16* STMFLASH_BUF = NULL;
u16* iapbuf = NULL;
//����״̬
//bit15��	������ɱ�־
//bit14��	���յ�0x0d
//bit13~0��	���յ�����Ч�ֽ���Ŀ
u32 USART_RX_STA=0;       //����״̬���	  

extern u8 g_printf_enable;
extern u8 g_ota_sta;
extern u32 g_ota_recv_sum;
extern u16 g_ota_pg_numid;

extern u16 g_ota_pg_nums;
extern u32 g_ota_bin_size;

#define STM_SECTOR_SIZE	2048

extern int EXT_MAX_SS_MIN;
extern int EXT_MAX_SS_MAX;

u8 snd_buf[128] = {0};

extern u8 g_ota_bin_md5[SSL_MAX_LEN];
extern u8 g_ota_package_md5[SSL_MAX_LEN];

u8 g_after_rx_intr = 0;

void UART1_SendData(u8 *data, u16 num);

//��ʼ��IO ����1 
//bound:������
void uart_init(u32 bound){
    //GPIO�˿�����
    GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	 
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1|RCC_APB2Periph_GPIOA, ENABLE);	//ʹ��USART1��GPIOAʱ��
 	USART_DeInit(USART1);  //��λ����1
	//USART1_TX   PA.9
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9; //PA.9
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//�����������
    GPIO_Init(GPIOA, &GPIO_InitStructure); //��ʼ��PA9
   
    //USART1_RX	  PA.10
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//��������
    GPIO_Init(GPIOA, &GPIO_InitStructure);  //��ʼ��PA10

   //Usart1 NVIC ����

    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3 ;//��ռ���ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//�����ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ���
  
   //USART ��ʼ������
	USART_InitStructure.USART_BaudRate = bound;//һ������Ϊ9600;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//�ֳ�Ϊ8λ���ݸ�ʽ
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//һ��ֹͣλ
	USART_InitStructure.USART_Parity = USART_Parity_No;//����żУ��λ
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//��Ӳ������������
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//�շ�ģʽ

    USART_Init(USART1, &USART_InitStructure); //��ʼ������
    //USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);//�����ж�
		USART_ITConfig(USART1, USART_IT_IDLE, ENABLE);//���������ж�
    USART_Cmd(USART1, ENABLE);                    //ʹ�ܴ��� 
		
		while(USART_GetFlagStatus(USART1,USART_FLAG_TC) == RESET);
		
		// 100ms
		TIM6_Int_Init(9999, 7199);
		
		TIM_Cmd(TIM6, DISABLE); //�رն�ʱ��7
		
		USART_RX_STA=0;
		
		USART_RX_BUF = (u8*)mymalloc(SRAMEX, USART_MAX_RECV_LEN);
		USART_RX_BUF_BAK = (u8*)mymalloc(SRAMEX, USART_MAX_RECV_LEN);
		
		iapbuf = (u16*)mymalloc(SRAMEX, 1024*2);
		
		STMFLASH_BUF = (u16*)mymalloc(SRAMEX, (STM_SECTOR_SIZE/2)*2);//�����2K�ֽ�
}

void run_cmd_from_usart(u8 *data, u16 num)
{
	u8 snd_len = 0;
	
	if (data[0] != 0xfe) {
		return;
	}

	snd_len = num;
	memset(snd_buf, 0, 128);

	switch (data[1])
	{
		case 0x55:// Enable Printf Debug Info
			// FE 55 00 FF -> disable
			// FE 55 01 FF -> enable
			g_printf_enable = data[2];
			break;
		case 0x40:// OTA
			switch(data[2])
			{
				// FE 40 01 01 FF
				case 0x01:// OTA Start
					// TBD: Clost FAN / MOT / HOT
					g_ota_sta = 3;
					g_ota_recv_sum = 0;
					g_ota_pg_numid = 0;
					g_ota_pg_nums = 0;
					g_ota_bin_size = 0;
					memcpy(snd_buf, data, num);
					break;
				// FE 40 02 + 3B-Size + 1B-TotalPackagesNum + MD5 + FF
				case 0x02:// Total Packages & MD5
					g_ota_pg_nums = data[6];
					g_ota_bin_size = (data[3]<<16) + (data[4]<<8) + data[5];
					memcpy(g_ota_bin_md5, data+7, SSL_MAX_LEN);
					memcpy(snd_buf, data, num);
					break;
				// After Recved 03 ACK, APP will send the package data
				// FE 40 03 + 1B-PackageNum + MD5 + FF
				case 0x03:// Divided Package & MD5
					g_ota_pg_numid = data[3];
					memcpy(g_ota_package_md5, data+4, SSL_MAX_LEN);
					memcpy(snd_buf, data, num);
					break;
				// FE 40 06 01 FF
				case 0x06:// Query OTA Sta
					memcpy(snd_buf, data, num);
					snd_buf[3] = g_ota_sta;// 0-idle, 1-pass, 2-fail, 3-in progress
					break;
				// FE 40 07 01 FF
				case 0x07:// Force to Stop OTA
					g_ota_sta = 2;
					g_ota_recv_sum = 0;
					g_ota_pg_numid = 0;
					g_ota_pg_nums = 0;
					g_ota_bin_size = 0;
					memcpy(snd_buf, data, num);
					break;
				default:
					snd_len = 0;
					break;
			}
			break;
		default:
			snd_len = 0;
			break;
	}
	
	if (snd_len != 0) {
		UART1_SendData(snd_buf, snd_len);
	}
}

void UART1_SendData(u8 *data, u16 num)
{
	u16 t = 0;
	u16 len = num;
	
	for(t=0;t<len;t++)
	{
		USART_SendData(USART1, data[t]);
		while(USART_GetFlagStatus(USART1,USART_FLAG_TC)!=SET);
	}
}

void UART1_ButtonOtaReport(void)
{
	u8 buf[8] = {0xFF, 0x40, 0x08, 0x01, 0xFF};

	UART1_SendData(buf, 5);
}

void UART1_ReportMcuNeedReset(void)
{
	u8 buf[8] = {0xFF, 0x05, 0x01, 0xFF};

	UART1_SendData(buf, 4);
}

// FE 40 04 + 1B-RESULT + 1B-PackageNum + FF
// RESULT: 1-pass, 2-fail
void UART1_ReportOtaPackageSta(u8 md5_res)
{
	u8 buf[8] = {0xFF, 0x40, 0x04, 0x01, 0x01, 0xFF};// Pass

	if (md5_res != 1) {
		buf[3] = 2;// Fail
	}

	buf[4] = g_ota_pg_numid;

	UART1_SendData(buf, 6);
}

// MCU request to stop ota
void UART1_RequestStopOta(void)
{
	u8 buf[5] = {0xFF, 0x40, 0x08, 0x01, 0xFF};

	UART1_SendData(buf, 5);
}

void UART1_ReportOtaBinSta(u8 md5_res)
{
	u8 buf[5] = {0xFF, 0x40, 0x05, 0x01, 0xFF};// Pass

	if (md5_res != 1) {
		buf[3] = 2;// Fail
	}

	UART1_SendData(buf, 5);
}

void USART1_IRQHandler(void)                	//����1�жϷ������
{
#ifdef SYSTEM_SUPPORT_OS	 	
	OSIntEnter();    
#endif

		 if (USART_GetITStatus(USART1, USART_IT_IDLE) != RESET)  
	   {
				g_after_rx_intr = 1;
				TIM_SetCounter(TIM6,0);
			  TIM_Cmd(TIM6, ENABLE);//ʹ�ܶ�ʱ��7

	      USART_ClearITPendingBit(USART1, USART_IT_IDLE);         //����жϱ�־
	      USART_ReceiveData(USART1);//��ȡ���� ע�⣺������Ҫ�������ܹ�����жϱ�־λ��
	   }

#ifdef SYSTEM_SUPPORT_OS	 
	OSIntExit();  											 
#endif
} 
#endif	

