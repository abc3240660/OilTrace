#include "mcp3204.h" 
#include "spi.h"
#include "delay.h"   
//Mini STM32开发板
//MCP3204 驱动函数 


//初始化SPI FLASH的IO口
void SPI_MCP3204_Init(void)
{

	GPIO_InitTypeDef GPIO_InitStructure;
	
#ifdef CS_PB6
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;  //MCP3024 CS 
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  //复用推挽输出
 	GPIO_Init(GPIOB, &GPIO_InitStructure);
 	GPIO_SetBits(GPIOB ,GPIO_Pin_6);
#else
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;  //MCP3024 CS 
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  //复用推挽输出
 	GPIO_Init(GPIOB, &GPIO_InitStructure);
 	GPIO_SetBits(GPIOB ,GPIO_Pin_7);
#endif
	
	SPI3_Init();		   //初始化SPI
}  

u16 SPI_MCP3204_ReadData(u8 channel)   
{  
	u8 byte=0,valh,vall;  
	u16 retval;
	SPI_MCP3204_CS=0;                            //使能器件   
#if 0
	SPI3_ReadWriteByte(0x18+channel);
	valh=SPI3_ReadWriteByte(0x00);
	vall=SPI3_ReadWriteByte(0x00);
#else
	byte=(channel<<6) & 0xc0;; 
	SPI3_ReadWriteByte(0x6);    //发送读取状态寄存器命令    
	valh=SPI3_ReadWriteByte(byte);             //读取一个字节  	
	vall=SPI3_ReadWriteByte(0xff);
#endif
	SPI_MCP3204_CS=1;                            //取消片选       
	retval = valh & 0xf;
	retval = retval*256 + vall;
	return retval;
}
