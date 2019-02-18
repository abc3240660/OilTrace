#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "iap.h"
//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//ALIENTEKս��STM32������
//IAP ����	   
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//�޸�����:2012/9/24
//�汾��V1.0
//��Ȩ���У�����ؾ���
//Copyright(C) ������������ӿƼ����޹�˾ 2009-2019
//All rights reserved									  
//////////////////////////////////////////////////////////////////////////////////	

iapfun jump2app; 
extern u16* iapbuf;
//appxaddr:Ӧ�ó������ʼ��ַ
//appbuf:Ӧ�ó���CODE.
//appsize:Ӧ�ó����С(�ֽ�).
void iap_write_appbin(u32 appxaddr,u8 *appbuf,u32 appsize)
{
	u16 t;
	u16 i=0;
	u16 temp;
	u32 fwaddr=appxaddr;//��ǰд��ĵ�ַ
	u8 *dfu=appbuf;
	for(t=0;t<appsize;t+=2)
	{						    
		temp=(u16)dfu[1]<<8;
		temp+=(u16)dfu[0];	  
		dfu+=2;//ƫ��2���ֽ�
		iapbuf[i++]=temp;	    
		if(i==1024)
		{
			i=0;
			STMFLASH_Write(fwaddr,iapbuf,1024);	
			fwaddr+=2048;//ƫ��2048  16=2*8.����Ҫ����2.
		}
	}
	if(i)STMFLASH_Write(fwaddr,iapbuf,i);//������һЩ�����ֽ�д��ȥ.  
}

//��ת��Ӧ�ó����
//appxaddr:�û�������ʼ��ַ.
void iap_load_app(u32 appxaddr)
{
	if(((*(vu32*)appxaddr)&0x2FFE0000)==0x20000000)	//���ջ����ַ�Ƿ�Ϸ�.
	{ 
		jump2app=(iapfun)*(vu32*)(appxaddr+4);		//�û��������ڶ�����Ϊ����ʼ��ַ(��λ��ַ)		
		MSR_MSP(*(vu32*)appxaddr);					//��ʼ��APP��ջָ��(�û��������ĵ�һ�������ڴ��ջ����ַ)
		jump2app();									//��ת��APP.
	}
}	

void download_success(void)
{
	recv_bin_success();
}

u8  UPGRADE[4]={0x1A,0x1A,0x2B,0x2B}; //�̼���Ҫ���±�־0x1A 0x1A 0x2B 0x2B
u16	flashBUF[512]; //����FLASH�洢��������;1K�ֽڴ洢����ΪSTM32�ڲ�FLASH��ȡΪ����(u16)��д������Ϊ���ʺͶ�ʱ���

void UP_success(void)
{
	u8 i;
	u32 j,k=0;
	//if(*(u32*)UPDATE_PARAM_SAVE_ADDR_BASE==0x2B2B1A1A)//��������1A 1A 2B 2B �õ����µİ汾��
	//{
		flashBUF[0] = 0x3c3c;
		flashBUF[1] = 0x4d4d;
		FLASH_Unlock();
		FLASH_ClearFlag(FLASH_FLAG_EOP|FLASH_FLAG_PGERR|FLASH_FLAG_WRPRTERR);
		FLASH_ErasePage(UPDATE_PARAM_SAVE_ADDR_BASE);												//�Ȳ���д
		for(i=0;i<2;i++)																		//u8���u16����ֽ�����					   
		{
			FLASH_ProgramHalfWord((UPDATE_PARAM_SAVE_ADDR_BASE+i*2),flashBUF[i]);//flash  Ϊһ���ֽڴ洢��16λ���ݱ����ַ��2
		}
		FLASH_Lock();
	//}
}
