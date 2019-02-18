#ifndef __AD_H
#define __AD_H
#include "common.h"

#define SPI_CLK_HI  21    //20uS
#define AD_I_FULL_RANGE  (uint32_t)393216       //20mA*100(R=120)
//#define AD_V_FULL_RANGE  (uint32_t)1905      //5000mV(R=1K/3.3K)
#define AD_V_FULL_RANGE  (uint32_t)4096      //5000mV(R=1K/1K)


#define AD_TOTAL_CHANNELS 8

#define SPI_LED_PORT    GPIOB
#define SPI_CS1_PORT    GPIOB
#define SPI_CS2_PORT    GPIOB
#define SPI_SCK_PORT   GPIOB
#define SPI_MOSI_PORT  GPIOB
#define SPI_MISO_PORT  GPIOB

#define SPI_LED_PIN    	GPIO_Pin_13

#define SPI_CS1_PIN    	GPIO_Pin_6
#define SPI_CS2_PIN     GPIO_Pin_7
#define SPI_SCK_PIN   	GPIO_Pin_3
#define SPI_MOSI_PIN  	GPIO_Pin_5
#define SPI_MISO_PIN  	GPIO_Pin_4

#define SPI_CS1_H()         GPIO_SetBits(SPI_CS1_PORT, SPI_CS1_PIN)
#define SPI_CS1_L()         GPIO_ResetBits(SPI_CS1_PORT, SPI_CS1_PIN)
#define SPI_CS2_H()         GPIO_SetBits(SPI_CS2_PORT, SPI_CS2_PIN)
#define SPI_CS2_L()         GPIO_ResetBits(SPI_CS2_PORT, SPI_CS2_PIN)
#define SPI_SCK_H()        GPIO_SetBits(SPI_SCK_PORT, SPI_SCK_PIN)
#define SPI_SCK_L()        GPIO_ResetBits(SPI_SCK_PORT, SPI_SCK_PIN)
#define SPI_MOSI_H()       GPIO_SetBits(SPI_MOSI_PORT, SPI_MOSI_PIN)
#define SPI_MOSI_L()       GPIO_ResetBits(SPI_MOSI_PORT, SPI_MOSI_PIN)
#define SPI_MISO()         (GPIO_ReadInputDataBit(SPI_MISO_PORT, SPI_MISO_PIN))  

void Delay(u16 nCount);
u16 SPI_Read(u8 ch);
void upDataAnalog(u16 value,u8 ch);
void AD3204_Init(void);

#endif