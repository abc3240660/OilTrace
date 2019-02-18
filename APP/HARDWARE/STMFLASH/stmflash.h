#ifndef __STMFLASH_H__
#define __STMFLASH_H__
#include "sys.h"  
//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã�δ���������ɣ��������������κ���;
//ALIENTEKս��STM32������
//STM32 FLASH ��������	   
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//�޸�����:2012/9/13
//�汾��V1.0
//��Ȩ���У�����ؾ���
//Copyright(C) �������������ӿƼ����޹�˾ 2009-2019
//All rights reserved									  
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////////
//�û������Լ�����Ҫ����
#define STM32_FLASH_SIZE 1024 	 		//��ѡSTM32��FLASH������С(��λΪK)
#define STM32_FLASH_WREN 1              //ʹ��FLASHд��(0��������;1��ʹ��)
//////////////////////////////////////////////////////////////////////////////////////////////////////

//FLASH��ʼ��ַ
#define STM32_FLASH_BASE 0x08000000 	//STM32 FLASH����ʼ��ַ
//FLASH������ֵ

#define SYS_Bootloader_SAVE_ADDR_BASE       0x08000000// Bootloader

#define UPDATE_PARAM_SAVE_ADDR_BASE         0x08018800// offset:98KB
#define UPDATE_PARAM_MAX_SIZE               (2*1024)// size:2KB

#define SYS_APP_SAVE_ADDR_BASE              0x08019000// offset:100KB
#define APP_DATA_MAX_SIZE                   (200*1024)// size:200KB = 0x32000
// 200KB APP1
#define SYS_APP_BAK_SAVE_ADDR_BASE          0x0804B000// offset:300KB
#define APP_BAK_DATA_MAX_SIZE               (200*1024)// size:200KB = 0x32000
// 200KB BAK

#define FLASH_BAK_ADDR		SYS_APP_BAK_SAVE_ADDR_BASE  	//��һ��Ӧ�ó�����ʼ��ַ(�����FLASH)

u16 STMFLASH_ReadHalfWord(u32 faddr);		  //��������  
void STMFLASH_WriteLenByte(u32 WriteAddr,u32 DataToWrite,u16 Len);	//ָ����ַ��ʼд��ָ�����ȵ�����
u32 STMFLASH_ReadLenByte(u32 ReadAddr,u16 Len);						//ָ����ַ��ʼ��ȡָ����������
void STMFLASH_Write(u32 WriteAddr,u16 *pBuffer,u16 NumToWrite);		//��ָ����ַ��ʼд��ָ�����ȵ�����
void STMFLASH_Read(u32 ReadAddr,u16 *pBuffer,u16 NumToRead);   		//��ָ����ַ��ʼ����ָ�����ȵ�����
void recv_bin_success(void);
//����д��
void Test_Write(u32 WriteAddr,u16 WriteData);								   
#endif
















