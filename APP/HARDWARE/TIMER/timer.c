#include "timer.h"
#include "led.h"
#include "usart.h"
#include "includes.h"	
#include "dma.h"
//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//ALIENTEK ս��������
//��ʱ�� ��������	   
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//��������:2015/1/13
//�汾��V1.0
//��Ȩ���У�����ؾ���
//Copyright(C) ������������ӿƼ����޹�˾ 2014-2024
//All rights reserved									  
////////////////////////////////////////////////////////////////////////////////// 	 

extern u8 g_ota_sta;
extern u32 g_ota_recv_sum;
extern u16 g_ota_pg_numid;

extern u8 g_after_rx_intr;

u32 os_jiffies = 0;
u32 os_jiffies_10ms = 0;

void TIM3_Int_Init(u16 arr,u16 psc)
{
	TIM_TimeBaseInitTypeDef	TIM_TimeBaseInitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3,ENABLE);//����TIM3ʱ�� 

	TIM_TimeBaseInitStructure.TIM_Prescaler=psc;   //��Ƶֵ
	TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up;	   //����ģʽ
	TIM_TimeBaseInitStructure.TIM_Period=arr;		   //�Զ���װ��ֵ
	TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1;  //����ʱ�ӷָ�
	TIM_TimeBaseInit(TIM3,&TIM_TimeBaseInitStructure);
	TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE);//��������ж�

	NVIC_InitStructure.NVIC_IRQChannel=TIM3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority=3;
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	TIM_Cmd(TIM3,ENABLE);		  //ʹ��TIM3
}

// 10ms
void TIM3_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM3,TIM_IT_Update)!=RESET)
	{
		os_jiffies_10ms++;
		if (os_jiffies_10ms >= 10000) {
			os_jiffies_10ms = 0;
		}

		if (0 == (os_jiffies_10ms%10)) {
			// printf("os_jiffies_10ms = %d\n", os_jiffies_10ms);
			os_jiffies++;
			if (os_jiffies >= 10000) {
				os_jiffies = 0;
			}
		}
	}
	TIM_ClearITPendingBit(TIM3,TIM_IT_Update);
}

//������ʱ��6�жϳ�ʼ��
//����ʱ��ѡ��ΪAPB1��2������APB1Ϊ36M
//arr���Զ���װֵ��
//psc��ʱ��Ԥ��Ƶ��
//����ʹ�õ��Ƕ�ʱ��6!
void TIM6_Int_Init(u16 arr,u16 psc)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6,ENABLE); //��ʱ��6ʱ��ʹ��
	
	TIM_TimeBaseInitStructure.TIM_Prescaler=psc;  //���÷�Ƶֵ��10khz�ļ���Ƶ��
	TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up; //���ϼ���ģʽ
	TIM_TimeBaseInitStructure.TIM_Period=arr;  //�Զ���װ��ֵ ������5000Ϊ500ms
	TIM_TimeBaseInitStructure.TIM_ClockDivision=0; //ʱ�ӷָ�:TDS=Tck_Tim
	TIM_TimeBaseInit(TIM6,&TIM_TimeBaseInitStructure);
	
	TIM_ITConfig(TIM6,TIM_IT_Update,ENABLE); //ʹ��TIM6�ĸ����ж�

	NVIC_InitStructure.NVIC_IRQChannel=TIM6_IRQn; //TIM6�ж�
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=1; //��ռ���ȼ�1��
	NVIC_InitStructure.NVIC_IRQChannelSubPriority=3;  //�����ȼ�3��
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE; //ʹ��ͨ��
	NVIC_Init(&NVIC_InitStructure);
}


extern u32 USART_RX_STA;
void TIM6_IRQHandler(void)
{
	OSIntEnter();
	if(TIM_GetITStatus(TIM6,TIM_IT_Update)!=RESET)
	{
		if (1 == g_after_rx_intr) {
			u16 re_count = 0;
		
			DMA_Cmd(DMA1_Channel5, DISABLE);
			re_count = U1_DMA_R_LEN - DMA_GetCurrDataCounter(DMA1_Channel5);
			
			if((USART_RX_STA&(1<<19))==0)
			{ 
				if(USART_RX_STA<USART_MAX_RECV_LEN)
				{
					if (re_count < (USART_MAX_RECV_LEN - USART_RX_STA)) {
						memcpy(USART_RX_BUF+(USART_RX_STA&0xFFFF), &U1_DMA_R_BUF[0], re_count);
						USART_RX_STA += re_count;
					} else {
						memcpy(USART_RX_BUF+(USART_RX_STA&0xFFFF), &U1_DMA_R_BUF[0], (USART_MAX_RECV_LEN - USART_RX_STA));
						USART_RX_STA += (USART_MAX_RECV_LEN - USART_RX_STA);
						USART_RX_STA|=1<<19;
					}
				}else 
				{
					USART_RX_STA|=1<<19;
				}
			} else {
				// Buffer Overflow
				// Error MSG printf if NEED
				USART_RX_STA|=1<<19;
			}

			USART_RX_STA|=1<<19;
#if 1
		{
			u16 i = 0;
			for (i=0; i<(USART_RX_STA&0xFFFF); i++) {
				if (USART_RX_BUF[i] != 0x88) {
					USART_RX_STA = 0;
				}
			}
			USART_RX_STA = 0;
			DMA_SetCurrDataCounter(DMA1_Channel5, U1_DMA_R_LEN);
			DMA_Cmd(DMA1_Channel5, ENABLE);
		}
#endif
		}

		// printf("Recved Data(%dB) %.2X-%.2X\n", (USART_RX_STA&0xFFFF), USART_RX_BUF[0], USART_RX_BUF[((USART_RX_STA&0xFFFF) - 1)]);
		
	  TIM_ClearITPendingBit(TIM6,TIM_IT_Update); //����жϱ�־λ
		
		TIM_Cmd(TIM6,DISABLE);
	}
	OSIntExit();
}




