#include  "ad.h"
u16 Analog_Data_Buf[AD_TOTAL_CHANNELS]={0x05DC,0x0000,0x0000,0x0000,
                                      0x099C,0x0000,0x0000,0x0000};

void Delay(u16 nCount)
{
  /* Decrement nCount value */
  while (nCount != 0)
  {
    nCount--;
  }
}

void SPI_CS_H(u8 ch)
{
	if (ch < 8) {
		SPI_CS1_H();
	} else {
		SPI_CS2_H();
	}
}

void SPI_CS_L(u8 ch)
{
	if (ch < 8) {
		SPI_CS1_L();
	} else {
		SPI_CS2_L();
	}
}

u16 SPI_Read(u8 ch)
{
	u8 i;
	u8 ch_all;
	u16  data1 = 0;
 	u16  data2 = 0;

	SPI_CS1_H();
	SPI_CS2_H();
	
	ch_all = ch;
	if (ch > 7) {
		ch = ch - 8;
	}
	
  if(ch>7)
  {
    return 0;
  }
  ch |=0x8;
  SPI_CS_H(ch_all);
  Delay(SPI_CLK_HI);
  SPI_SCK_L();
  SPI_MOSI_H();
  SPI_CS_L(ch_all);
  
  Delay(SPI_CLK_HI);
  for(i=0;i<4;i++)
  {
    Delay(SPI_CLK_HI);
    SPI_SCK_H();
    if(ch&0x08)
    {
      SPI_MOSI_H();
    }
    else
    {
      SPI_MOSI_L();
    }
    ch<<=1;
    Delay(SPI_CLK_HI);
    SPI_SCK_L();
  }
  Delay(SPI_CLK_HI);
  SPI_SCK_H();
  Delay(SPI_CLK_HI);
  SPI_SCK_L();      //1
  Delay(SPI_CLK_HI);
  SPI_SCK_H();
  Delay(SPI_CLK_HI);
  SPI_SCK_L();      //2
  Delay(SPI_CLK_HI);
    
  data1=0;
  data2=0;
  for(i=0;i<11;i++)
  {
    SPI_SCK_H();
    Delay(SPI_CLK_HI);
    SPI_SCK_L();
    Delay(SPI_CLK_HI);
    if(SPI_MISO())
    {
      data1|=0x0001;
    }
    else
    {
      data1&=0xFFFE;
    }
    data1<<=1;
  }
    SPI_SCK_H();
    Delay(SPI_CLK_HI);
    SPI_SCK_L();
    Delay(SPI_CLK_HI);
    if(SPI_MISO())
    {
      data1|=0x001;
      data2|=0x800;    
    }
    else
    {
      data1&=0xFFE;
      data2&=0x7FF;    
    }
  
  for(i=0;i<11;i++)
  {
    data2>>=1;
    SPI_SCK_H();
    Delay(SPI_CLK_HI);
    SPI_SCK_L();
    Delay(SPI_CLK_HI);
    if(SPI_MISO())
    {
      data2|=0x800;    
    }
    else
    {
      data2&=0x7FF;    
    }
  }
  SPI_CS_H(ch_all);
    
  Delay(100);
  return (data2&0xFFF);
  
}


void upDataAnalog(u16 value,u8 ch)
{
  if(ch>7)
  {
    return;
  }
  if(ch<4)
  {
    Analog_Data_Buf[ch]=(u16)(((((float)value*2000000))/AD_I_FULL_RANGE)+0.5);
  }
  else
  {
    Analog_Data_Buf[ch]=(u16)(((((float)value*5000))/AD_V_FULL_RANGE)+0.5);
  }
}

void AD3204_Init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB|RCC_APB2Periph_AFIO, ENABLE);
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);

	// MISO
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
 	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	// CLK & CS & MOSI
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3|GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7;
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
 	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	GPIO_ResetBits(GPIOB, GPIO_Pin_3|GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7);
}
