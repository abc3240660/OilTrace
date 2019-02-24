#ifndef __USART_H
#define __USART_H
#include "stdio.h"	
#include "sys.h" 
//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//Mini STM32������
//����1��ʼ��		   
//����ԭ��@ALIENTEK
//������̳:www.openedv.csom
//�޸�����:2011/6/14
//�汾��V1.4
//��Ȩ���У�����ؾ���
//Copyright(C) ����ԭ�� 2009-2019
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
////////////////////////////////////////////////////////////////////////////////// 	
#define USART_MAX_RECV_LEN  			1024*4  	//�����������ֽ��� 200
#define UART5_MAX_SEND_LEN				400

#define USART_REC_LEN  			4096  	//�����������ֽ��� 200

#define EN_USART1_RX 			1		//ʹ�ܣ�1��/��ֹ��0������1����

//ģʽ����
#define RS485_TX_EN		PAout(8)	//485ģʽ����.0,����;1,����.

#define FILE_MD5_MAX_LEN  32
#define SSL_MAX_LEN (FILE_MD5_MAX_LEN/2)

extern u8  UART5_TX_BUF[UART5_MAX_SEND_LEN];
extern u8* USART_RX_BUF;
extern u8* USART_RX_BUF2;
extern u8* USART_RX_BUF_BAK;
extern u32 USART_RX_STA;         		//����״̬���	
extern u32 USART_RX_STA2;
//����봮���жϽ��գ��벻Ҫע�����º궨��
void uart_init(u32 bound);
void uart2_init(u32 bound);
void u1_printf(char* fmt,...);
void UART1_AdValReport(u8 ch, u16 *val);
void UART1_ReportTestSta(void);
void UART1_HeartBeat(void);
void UART1_ParamsRequest(void);
void UART1_AdValReportOffline(u8 ch, u16 *val);

#endif


