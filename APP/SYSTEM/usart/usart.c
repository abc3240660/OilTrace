#include "sys.h"
#include "usart.h"	 
#include "dma.h"
#include "common.h"
#include "timer.h"
#include "malloc.h"
#include "gagent_md5.h"
#include "rtc.h"
////////////////////////////////////////////////////////////////////////////////// 	 
//如果使用os,则包括下面的头文件即可.
#if SYSTEM_SUPPORT_OS
#include "includes.h"					//os 使用	  
#endif
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK STM32开发板
//串口1初始化		   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//修改日期:2012/8/18
//版本：V1.5
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2009-2019
//All rights reserved
//********************************************************************************
//V1.3修改说明 
//支持适应不同频率下的串口波特率设置.
//加入了对printf的支持
//增加了串口接收命令功能.
//修正了printf第一个字符丢失的bug
//V1.4修改说明
//1,修改串口初始化IO的bug
//2,修改了USART_RX_STA,使得串口最大接收字节数为2的14次方
//3,增加了USART_MAX_RECV_LEN,用于定义串口最大允许接收的字节数(不大于2的14次方)
//4,修改了EN_USART1_RX的使能方式
//V1.5修改说明
//1,增加了对UCOSII的支持
////////////////////////////////////////////////////////////////////////////////// 	  
 
__align(8) u8 UART5_TX_BUF[UART5_MAX_SEND_LEN];
//////////////////////////////////////////////////////////////////
//加入以下代码,支持printf函数,而不需要选择use MicroLIB	  
#if 1
#pragma import(__use_no_semihosting)             
//标准库需要的支持函数                 
struct __FILE 
{ 
	int handle; 

}; 

FILE __stdout;       
//定义_sys_exit()以避免使用半主机模式    
_sys_exit(int x) 
{ 
	x = x; 
} 
//重定义fputc函数 
int fputc(int ch, FILE *f)
{      
	while((USART2->SR&0X40)==0);//循环发送,直到发送完毕   
    USART2->DR = (u8) ch;      
	return ch;
}
#endif 
 
#if EN_USART1_RX   //如果使能了接收
//串口1中断服务程序
//注意,读取USARTx->SR能避免莫名其妙的错误   	
u8* USART_RX_BUF = NULL;
u8* USART_RX_BUF2 = NULL;
u8* USART_RX_BUF_BAK = NULL;
u16* STMFLASH_BUF = NULL;
u16* iapbuf = NULL;
//接收状态
//bit15，	接收完成标志
//bit14，	接收到0x0d
//bit13~0，	接收到的有效字节数目
u32 USART_RX_STA=0;       //接收状态标记	  
u32 USART_RX_STA2=0;

extern u8 g_printf_enable;
extern u8 g_ota_sta;
extern u32 g_ota_recv_sum;
extern u16 g_ota_pg_numid;

extern u16 g_ota_pg_nums;
extern u32 g_ota_bin_size;

#define STM_SECTOR_SIZE	2048

extern int EXT_MAX_SS_MIN;
extern int EXT_MAX_SS_MAX;

u8 snd_buf[128] = {0};

extern u8 g_ota_bin_md5[SSL_MAX_LEN];
extern u8 g_ota_package_md5[SSL_MAX_LEN];

u8 g_after_rx_intr = 0;

void UART1_SendData(u8 *data, u16 num);

//初始化IO 串口1 
//bound:波特率
void uart_init(u32 bound){
	//GPIO端口设置
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1|RCC_APB2Periph_GPIOA, ENABLE);	//使能USART1，GPIOA时钟
	USART_DeInit(USART1);  //复位串口1
	//USART1_TX   PA.9
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9; //PA.9
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//复用推挽输出
	GPIO_Init(GPIOA, &GPIO_InitStructure); //初始化PA9
	
	//USART1_RX	  PA.10
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//浮空输入
	GPIO_Init(GPIOA, &GPIO_InitStructure);  //初始化PA10
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;				 //PD7端口配置
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 //推挽输出
 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
 	GPIO_Init(GPIOA, &GPIO_InitStructure);

	//Usart1 NVIC 配置
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3 ;//抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器
	
	//USART 初始化设置
	USART_InitStructure.USART_BaudRate = bound;//一般设置为9600;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式
	
	USART_Init(USART1, &USART_InitStructure); //初始化串口
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);//开启中断
	//USART_ITConfig(USART1, USART_IT_IDLE, ENABLE);//开启空闲中断
	USART_Cmd(USART1, ENABLE);                    //使能串口 

	RS485_TX_EN=1;// SEND MODE
	while(USART_GetFlagStatus(USART1,USART_FLAG_TC) == RESET);
	
	// 100ms
	// TIM6_Int_Init(999, 7199);
	
	// TIM_Cmd(TIM6, DISABLE); //关闭定时器7
	
	USART_RX_STA=0;
	USART_RX_STA2=0;
	
	USART_RX_BUF = (u8*)mymalloc(SRAMIN, USART_MAX_RECV_LEN);
	USART_RX_BUF2 = (u8*)mymalloc(SRAMIN, USART_MAX_RECV_LEN);
	USART_RX_BUF_BAK = (u8*)mymalloc(SRAMIN, USART_MAX_RECV_LEN);
	
	iapbuf = (u16*)mymalloc(SRAMIN, 1024*2);
	
	STMFLASH_BUF = (u16*)mymalloc(SRAMIN, (STM_SECTOR_SIZE/2)*2);//最多是2K字节
}

void uart2_init(u32 bound){
	//GPIO端口设置
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	//使能USART1，GPIOA时钟
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);	//使能USART1，GPIOA时钟
	
	USART_DeInit(USART2);  //复位串口1
	//USART1_TX   PA.2
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2; //PA.2
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//复用推挽输出
	GPIO_Init(GPIOA, &GPIO_InitStructure); //初始化PA2
	
	//USART1_RX	  PA.3
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//浮空输入
	GPIO_Init(GPIOA, &GPIO_InitStructure);  //初始化PA10
	
	//USART 初始化设置
	USART_InitStructure.USART_BaudRate = bound;//一般设置为9600;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式
	
	USART_Init(USART2, &USART_InitStructure); //初始化串口
	//USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);//开启中断
	//USART_ITConfig(USART2, USART_IT_IDLE, ENABLE);//开启空闲中断
	USART_Cmd(USART2, ENABLE);                    //使能串口 
	
	while(USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET);
}

void UART1_SendData(u8 *data, u16 num)
{
	u16 t = 0;
	u16 len = num;
	u16 try_cnt = 0;
	u32 temp_sta = 0;

	while(USART_RX_STA != 0) {
		if (temp_sta != USART_RX_STA) {
			try_cnt = 0;
			temp_sta = USART_RX_STA;
		} else {
			if (try_cnt++ >= 2000) {
				break;
			}
		}
		delay_ms(1);
	}

	RS485_TX_EN=1;// SEND MODE

	for(t=0;t<len;t++)
	{
		USART_SendData(USART1, data[t]);
		while(USART_GetFlagStatus(USART1,USART_FLAG_TC)!=SET);
	}

	RS485_TX_EN=0;// RECV MODE
}

void UART1_AdValReport(float *val)
{
	u8 i = 0;
	u8 flag = 0;
	u32* p_cid =  NULL;
	u8 buf[80] = {0};

	u8 decimal_val = 0;
	u16 integer_val = 0;
	
	u8* package_md5_calc =  NULL;
	MD5_CTX package_md5_ctx;
	
	buf[0] = 'S';
	buf[1] = 'T';
	buf[2] = 'A';
	
	p_cid = (u32*)(buf+3);
	*p_cid = *(vu32*)(0x1ffff7e8);
	
	p_cid = (u32*)(buf+7);
	*p_cid = *(vu32*)(0x1ffff7ec);
	
	p_cid = (u32*)(buf+11);
	*p_cid = *(vu32*)(0x1ffff7f0);

	buf[15] = 0x01;
	
//	for (i=0; i<12; i++) {
//		printf("%.2x", buf[3+i]);
//	}
//	printf("\n");

	for (i=0; i<22; i++) {
		integer_val = (u16)val[i];
		decimal_val = (u16)((val[i]-integer_val)*100);

		buf[16+i*2] = (u8)(integer_val>>1);
		buf[16+i*2+1] = (u8)((integer_val&0x01)*128 + decimal_val);
		
		if (i>=16) {
			if ((buf[16+i*2]!=0) || (buf[16+i*2+1]!=0)) {
				flag = 1;
			}
		}
	}
	
	if (1 == flag) {
		printf("EXTI DAT:");
		for (i=16; i<22; i++) {
			printf("%.2X%.2X", buf[16+i*2], buf[16+i*2+1]);
		}
		printf("\n");
	}
	
//	printf("UART1_AdValReport ...\n");

	package_md5_calc = buf+60;
	GAgent_MD5Init(&package_md5_ctx);
	GAgent_MD5Update(&package_md5_ctx, buf+3, 57);
  GAgent_MD5Final(&package_md5_ctx, package_md5_calc);
	
	buf[76] = 'E';
	buf[77] = 'N';
	buf[78] = 'D';

	UART1_SendData(buf, 79);
}

// FE 40 04 + 1B-RESULT + 1B-PackageNum + FF
// RESULT: 1-pass, 2-fail
void UART1_ReportOtaPackageSta(u8 md5_res)
{
	u32* p_cid =  NULL;
	u8 buf[40] = {0};

	u8* package_md5_calc =  NULL;
	MD5_CTX package_md5_ctx;
	
	buf[0] = 'S';
	buf[1] = 'T';
	buf[2] = 'A';
	
	p_cid = (u32*)(buf+3);
	*p_cid = *(vu32*)(0x1ffff7e8);
	
	p_cid = (u32*)(buf+7);
	*p_cid = *(vu32*)(0x1ffff7ec);
	
	p_cid = (u32*)(buf+11);
	*p_cid = *(vu32*)(0x1ffff7f0);

	buf[15] = 0x03;
	
	buf[16] = g_ota_pg_numid;
	buf[17] = md5_res;// result
	
	package_md5_calc = buf+18;
	GAgent_MD5Init(&package_md5_ctx);
	GAgent_MD5Update(&package_md5_ctx, buf+3, 15);
  GAgent_MD5Final(&package_md5_ctx, package_md5_calc);

	buf[34] = 'E';
	buf[35] = 'N';
	buf[36] = 'D';

	UART1_SendData(buf, 37);
}

void UART1_ReportTestSta(void)
{
	u8 buf[80] = {0};
	
	memset(buf, 0x55, 80);

	buf[0] = 'S';
	buf[1] = 'T';
	buf[2] = 'A';

	buf[15] = 0x03;
	
	buf[76] = 'E';
	buf[77] = 'N';
	buf[78] = 'D';

	UART1_SendData(buf, 79);
}

const char* SW_VER = "201903232235";

void UART1_ParamsRequest(void)
{
	u32* p_cid =  NULL;
	u8 buf[60] = {0};

	u8* package_md5_calc =  NULL;
	MD5_CTX package_md5_ctx;
	
	buf[0] = 'S';
	buf[1] = 'T';
	buf[2] = 'A';
	
	p_cid = (u32*)(buf+3);
	*p_cid = *(vu32*)(0x1ffff7e8);
	
	p_cid = (u32*)(buf+7);
	*p_cid = *(vu32*)(0x1ffff7ec);
	
	p_cid = (u32*)(buf+11);
	*p_cid = *(vu32*)(0x1ffff7f0);

	buf[15] = 0x06;
	
	buf[15+1] = ((SW_VER[0]-'0')<<4) + (SW_VER[1]-'0');
	buf[15+2] = ((SW_VER[2]-'0')<<4) + (SW_VER[3]-'0');
	buf[15+3] = ((SW_VER[4]-'0')<<4) + (SW_VER[5]-'0');
	buf[15+4] = ((SW_VER[6]-'0')<<4) + (SW_VER[7]-'0');
	buf[15+5] = ((SW_VER[8]-'0')<<4) + (SW_VER[9]-'0');
	buf[15+6] = ((SW_VER[10]-'0')<<4) + (SW_VER[11]-'0');

	package_md5_calc = buf+17+5;
	GAgent_MD5Init(&package_md5_ctx);
	GAgent_MD5Update(&package_md5_ctx, buf+3, 14+5);
  GAgent_MD5Final(&package_md5_ctx, package_md5_calc);

	buf[33+5] = 'E';
	buf[34+5] = 'N';
	buf[35+5] = 'D';

	UART1_SendData(buf, 36+5);
}

void parse_rtc_time(u8* buf)
{
	u8 i = 0;
	char tmp[64] = "";
	
	sprintf(tmp, "%.4d%.2d%.2d%.2d%.2d%.2d", calendar.w_year, calendar.w_month, calendar.w_date, calendar.hour, calendar.min, calendar.sec);
	
	for (i=0; i<7; i++) {
		buf[i] = ((tmp[2*i+0]-'0') << 4) + (tmp[2*i+1]-'0');
	}
}

void UART1_HeartBeat(void)
{
	u32* p_cid =  NULL;
	u8 buf[60] = {0};

	u8* package_md5_calc =  NULL;
	MD5_CTX package_md5_ctx;
	
	buf[0] = 'S';
	buf[1] = 'T';
	buf[2] = 'A';
	
	p_cid = (u32*)(buf+3);
	*p_cid = *(vu32*)(0x1ffff7e8);
	
	p_cid = (u32*)(buf+7);
	*p_cid = *(vu32*)(0x1ffff7ec);
	
	p_cid = (u32*)(buf+11);
	*p_cid = *(vu32*)(0x1ffff7f0);

	buf[15] = 0x07;
	
	RTC_Get();

	parse_rtc_time(buf+16);

	package_md5_calc = buf+16+7;
	GAgent_MD5Init(&package_md5_ctx);
	GAgent_MD5Update(&package_md5_ctx, buf+3, 13+7);
  GAgent_MD5Final(&package_md5_ctx, package_md5_calc);

	buf[32+7] = 'E';
	buf[33+7] = 'N';
	buf[34+7] = 'D';

	UART1_SendData(buf, 35+7);
}

void UART1_AdValReportOffline(OFFLINE_DAT *p_off_dat)
{
	u8 i = 0;
	u32* p_cid =  NULL;
	u8 buf[100] = {0};

	u8* package_md5_calc =  NULL;
	MD5_CTX package_md5_ctx;
	
	buf[0] = 'S';
	buf[1] = 'T';
	buf[2] = 'A';
	
	p_cid = (u32*)(buf+3);
	*p_cid = *(vu32*)(0x1ffff7e8);
	
	p_cid = (u32*)(buf+7);
	*p_cid = *(vu32*)(0x1ffff7ec);
	
	p_cid = (u32*)(buf+11);
	*p_cid = *(vu32*)(0x1ffff7f0);

	buf[15] = 0x05;
	
	memcpy(buf+16, p_off_dat->systime, 7);
	memcpy(buf+16+7, p_off_dat->mes_val, 44);
//	RTC_Get();
//	parse_rtc_time(buf+16);	
//	for (i=0; i<22; i++) {
//		buf[23+i*2] = (u8)(val[i]>>8);
//		buf[23+i*2+1] = (u8)(val[i]&0xFF);
//	}

	package_md5_calc = buf+67;
	GAgent_MD5Init(&package_md5_ctx);
	GAgent_MD5Update(&package_md5_ctx, buf+3, 64);
  GAgent_MD5Final(&package_md5_ctx, package_md5_calc);
	
	buf[83] = 'E';
	buf[84] = 'N';
	buf[85] = 'D';

	UART1_SendData(buf, 86);
}

void UART1_ReportOtaBinSta(u8 md5_res)
{
	u32* p_cid =  NULL;
	u8 buf[60] = {0};

	u8* package_md5_calc =  NULL;
	MD5_CTX package_md5_ctx;
	
	buf[0] = 'S';
	buf[1] = 'T';
	buf[2] = 'A';
	
	p_cid = (u32*)(buf+3);
	*p_cid = *(vu32*)(0x1ffff7e8);
	
	p_cid = (u32*)(buf+7);
	*p_cid = *(vu32*)(0x1ffff7ec);
	
	p_cid = (u32*)(buf+11);
	*p_cid = *(vu32*)(0x1ffff7f0);

	buf[15] = 0x04;
	
	buf[16] = md5_res;// result

	buf[16+1] = ((SW_VER[0]-'0')<<4) + (SW_VER[1]-'0');
	buf[16+2] = ((SW_VER[2]-'0')<<4) + (SW_VER[3]-'0');
	buf[16+3] = ((SW_VER[4]-'0')<<4) + (SW_VER[5]-'0');
	buf[16+4] = ((SW_VER[6]-'0')<<4) + (SW_VER[7]-'0');
	buf[16+5] = ((SW_VER[8]-'0')<<4) + (SW_VER[9]-'0');
	buf[16+6] = ((SW_VER[10]-'0')<<4) + (SW_VER[11]-'0');

	package_md5_calc = buf+17+6;
	GAgent_MD5Init(&package_md5_ctx);
	GAgent_MD5Update(&package_md5_ctx, buf+3, 14+6);
  GAgent_MD5Final(&package_md5_ctx, package_md5_calc);

	buf[33+6] = 'E';
	buf[34+6] = 'N';
	buf[35+6] = 'D';

	UART1_SendData(buf, 36+6);
}

void USART1_IRQHandler(void)                	//串口1中断服务程序
{
	u8 Res;
	u16 i = 0;

#if SYSTEM_SUPPORT_OS 		//如果SYSTEM_SUPPORT_OS为真，则需要支持OS.
	OSIntEnter();    
#endif
	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)  //接收中断(接收到的数据必须是0x0d 0x0a结尾)
	{
		Res =USART_ReceiveData(USART1);	//读取接收到的数据
		
		// USART_SendData(USART2, Res);
		
		if((USART_RX_STA&(1<<19))==0)//接收未完成
		{
			USART_RX_BUF[USART_RX_STA&0XFFFF]=Res ;
			USART_RX_STA++;

			if ('$' == Res) {
				if ('$' == USART_RX_BUF[(USART_RX_STA&0XFFFF)-2]) {
					if ('$' == USART_RX_BUF[(USART_RX_STA&0XFFFF)-3]) {
						if (strncmp((const char*)USART_RX_BUF,"STA",3) != 0) {
							USART_RX_STA = 0;// 接收数据错误,重新开始接收
						} else {
							if (strncmp((const char*)USART_RX_BUF+(USART_RX_STA&0XFFFF)-6,"END",3) != 0) {
								USART_RX_STA = 0;// 接收数据错误,重新开始接收
							} else {
								USART_RX_STA -= 3;
								USART_RX_STA|=1<<19;
								
								USART_RX_STA2 = USART_RX_STA;
								USART_RX_STA = 0;
								memcpy(USART_RX_BUF2, USART_RX_BUF, USART_RX_STA2&0XFFFF);
							}
						}
					}
				}
			}
			
			if((USART_RX_STA&0XFFFF)>(USART_REC_LEN-1))USART_RX_STA=0;// 接收数据错误,重新开始接收	  
		}	else {
			USART_RX_STA = 0;
		}
  } 
#if SYSTEM_SUPPORT_OS 	//如果SYSTEM_SUPPORT_OS为真，则需要支持OS.
	OSIntExit();  											 
#endif
}  
#endif	

void u1_printf(char* fmt,...)  
{
#if 0
	u16 i,j;
	va_list ap;
	va_start(ap,fmt);
	vsprintf((char*)UART5_TX_BUF,fmt,ap);
	va_end(ap);
	i=strlen((const char*)UART5_TX_BUF);
	
	RS485_TX_EN=1;// SEND MODE
	for(j=0;j<i;j++)
	{
		while(USART_GetFlagStatus(USART1,USART_FLAG_TC)==RESET); 
		USART_SendData(USART1,(uint8_t)UART5_TX_BUF[j]);   
	}
	RS485_TX_EN=0;// RECV MODE
//#else
	RS485_TX_EN=1;// SEND MODE
	RS485_TX_EN=0;// RECV MODE
#endif
}
