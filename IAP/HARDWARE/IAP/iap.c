#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "iap.h"
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK战舰STM32开发板
//IAP 代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//修改日期:2012/9/24
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2009-2019
//All rights reserved									  
//////////////////////////////////////////////////////////////////////////////////	

iapfun jump2app; 
extern u16* iapbuf;
//appxaddr:应用程序的起始地址
//appbuf:应用程序CODE.
//appsize:应用程序大小(字节).
void iap_write_appbin(u32 appxaddr,u8 *appbuf,u32 appsize)
{
	u16 t;
	u16 i=0;
	u16 temp;
	u32 fwaddr=appxaddr;//当前写入的地址
	u8 *dfu=appbuf;
	for(t=0;t<appsize;t+=2)
	{						    
		temp=(u16)dfu[1]<<8;
		temp+=(u16)dfu[0];	  
		dfu+=2;//偏移2个字节
		iapbuf[i++]=temp;	    
		if(i==1024)
		{
			i=0;
			STMFLASH_Write(fwaddr,iapbuf,1024);	
			fwaddr+=2048;//偏移2048  16=2*8.所以要乘以2.
		}
	}
	if(i)STMFLASH_Write(fwaddr,iapbuf,i);//将最后的一些内容字节写进去.  
}

//跳转到应用程序段
//appxaddr:用户代码起始地址.
void iap_load_app(u32 appxaddr)
{
	if(((*(vu32*)appxaddr)&0x2FFE0000)==0x20000000)	//检查栈顶地址是否合法.
	{ 
		jump2app=(iapfun)*(vu32*)(appxaddr+4);		//用户代码区第二个字为程序开始地址(复位地址)		
		MSR_MSP(*(vu32*)appxaddr);					//初始化APP堆栈指针(用户代码区的第一个字用于存放栈顶地址)
		jump2app();									//跳转到APP.
	}
}	

void download_success(void)
{
	recv_bin_success();
}

u8  UPGRADE[4]={0x1A,0x1A,0x2B,0x2B}; //固件需要更新标志0x1A 0x1A 0x2B 0x2B
u16	flashBUF[512]; //定义FLASH存储参数缓存;1K字节存储。因为STM32内部FLASH存取为半字(u16)读写，参数为功率和定时组合

void UP_success(void)
{
	u8 i;
	u32 j,k=0;
	//if(*(u32*)UPDATE_PARAM_SAVE_ADDR_BASE==0x2B2B1A1A)//读出的是1A 1A 2B 2B 用的是新的版本号
	//{
		flashBUF[0] = 0x3c3c;
		flashBUF[1] = 0x4d4d;
		FLASH_Unlock();
		FLASH_ClearFlag(FLASH_FLAG_EOP|FLASH_FLAG_PGERR|FLASH_FLAG_WRPRTERR);
		FLASH_ErasePage(UPDATE_PARAM_SAVE_ADDR_BASE);												//先擦后写
		for(i=0;i<2;i++)																		//u8变成u16后的字节数据					   
		{
			FLASH_ProgramHalfWord((UPDATE_PARAM_SAVE_ADDR_BASE+i*2),flashBUF[i]);//flash  为一个字节存储，16位数据必须地址加2
		}
		FLASH_Lock();
	//}
}
