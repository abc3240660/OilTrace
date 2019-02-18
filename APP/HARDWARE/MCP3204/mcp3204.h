#ifndef __MCP3204_H
#define __MCP3204_H			    
#include "sys.h" 

#define CS_PB6 1

#ifdef CS_PB6
#define	SPI_MCP3204_CS PBout(6)  //选中MCP3204	
#else
#define	SPI_MCP3204_CS PBout(7)  //选中MCP3204	
#endif

void SPI_MCP3204_Init(void);
u16 SPI_MCP3204_ReadData(u8 channel);
#endif
