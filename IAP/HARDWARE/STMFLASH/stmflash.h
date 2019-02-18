#ifndef __STMFLASH_H__
#define __STMFLASH_H__
#include "sys.h"  
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK战舰STM32开发板
//STM32 FLASH 驱动代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//修改日期:2012/9/13
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2009-2019
//All rights reserved									  
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////////
//用户根据自己的需要设置
#define STM32_FLASH_SIZE 1024 	 		//所选STM32的FLASH容量大小(单位为K)
#define STM32_FLASH_WREN 1              //使能FLASH写入(0，不是能;1，使能)
//////////////////////////////////////////////////////////////////////////////////////////////////////

//FLASH起始地址
#define STM32_FLASH_BASE 0x08000000 	//STM32 FLASH的起始地址
//FLASH解锁键值

#define SYS_Bootloader_SAVE_ADDR_BASE       0x08000000// Bootloader

#define UPDATE_PARAM_SAVE_ADDR_BASE         0x08018800// offset:98KB
#define UPDATE_PARAM_MAX_SIZE               (2*1024)// size:2KB

#define SYS_APP_SAVE_ADDR_BASE              0x08019000// offset:100KB
#define APP_DATA_MAX_SIZE                   (200*1024)// size:200KB = 0x32000
// 200KB APP1
#define SYS_APP_BAK_SAVE_ADDR_BASE          0x0804B000// offset:300KB
#define APP_BAK_DATA_MAX_SIZE               (200*1024)// size:200KB = 0x32000
// 200KB BAK

#define FLASH_BAK_ADDR		SYS_APP_BAK_SAVE_ADDR_BASE  	//第一个应用程序起始地址(存放在FLASH)

u16 STMFLASH_ReadHalfWord(u32 faddr);		  //读出半字  
void STMFLASH_WriteLenByte(u32 WriteAddr,u32 DataToWrite,u16 Len);	//指定地址开始写入指定长度的数据
u32 STMFLASH_ReadLenByte(u32 ReadAddr,u16 Len);						//指定地址开始读取指定长度数据
void STMFLASH_Write(u32 WriteAddr,u16 *pBuffer,u16 NumToWrite);		//从指定地址开始写入指定长度的数据
void STMFLASH_Read(u32 ReadAddr,u16 *pBuffer,u16 NumToRead);   		//从指定地址开始读出指定长度的数据
void recv_bin_success(void);
//测试写入
void Test_Write(u32 WriteAddr,u16 WriteData);								   
#endif

















