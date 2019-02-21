#ifndef __DMA_H
#define	__DMA_H	   
#include "sys.h"


#define FREE 0
#define USEING 1

#define U1_DMA_T_LEN	    70		//串口1最大发送缓存字节数
#define U1_DMA_R_LEN        512


extern u8 U1_DMA_R_BUF[U1_DMA_R_LEN];
//extern u8 U1_R_BUF[U1_DMA_R_LEN];
extern u8 U1_DMA_T_BUF[U1_DMA_T_LEN];
extern u8 U1_DMA_SEND_FREE_FLAG;
extern u32 System_Run_Counter;	//机器运行时间，开机开始，关机结束
extern u8 Key_Onoff_App_LongPressOK;	 //app发来off命令
extern u16 Key_Up_Down_App_Temp;	//app发送过来上下按键改变的温度
extern u8 Key_Up_App_ShortPressOK; //按键处理区分是app还是本地
extern u8 Key_Down_App_ShortPressOK; //按键处理区分是app还是本地
extern u8 Key_Direct_App_ShortPressOK; //按键处理区分是app还是本地
extern u8 Key_Prime_App_ContinuousPressOK; //按键处理区分是app还是本地
extern u8 Off_Temp_App;//接收到关机温度
extern u8 PID_PWM_App;//接收到PID和PWM参数


extern u8 Recipe_List[64];	
extern u8 Recipe_Start;
extern u16 Recipe_Time_Count;
extern u8 Recipe_Step;//1~9,0表示没有菜单
extern u8 Recipe_Beep;//收到菜单命令，蜂鸣器开始响


typedef struct 
{
  u8 fc;	  //F:1 C:0
  u8 feed;	  //Feed:0x01 no:0x00
  u8 onoff;	  //ON:0x01 Off:0x02
  u8 grill_set_act;	  //SET 0x01 ACT 0x02
  u8 probe_status;	  //都没插入0x00 |Probe1插入 0x01 |Probe 2插入 0x02 |都插入0x03
  u8 probe_num;	  //X:0x01 Y:0x02
}bbq_struct;
 
extern bbq_struct BBQ_Status_Struct;		  //目前机器运行的状态，屏幕的显示



void MYDMA_Config(DMA_Channel_TypeDef*DMA_CHx,u32 cpar,u32 cmar,u16 cndtr);//配置DMA1_CHx

void MYDMA_Enable(DMA_Channel_TypeDef*DMA_CHx);//使能DMA1_CHx

/***串口1配置****/
void Usart_DMA_Init(void);//串口1的DMA配置
void Uart1_DMA_Send_Array(u8 *buffer, u16 len);//发送数组

void Send_On_Message(void);
void Send_Off_Message(void);
void Send_Bypass_Message(void);
void Send_Feed_Start_Message(void);
void Send_Feed_End_Message(void);
void Send_Set_Temp_Message(u8 message_type,u8 temp_type,u16 tp);
void Send_Probe_Temp_Message(u8 probe_ch,u16 probe_x,u16 probe_y);
void Send_Probe_Statue_Message(u8 statue);
void Send_Temp_Type_Message(u8 type);
void Send_Display_Data_Message(u8 command);
void Send_All_Temp_Message(void);
void Set_Buf_Free(u8 *buf,u8 len); 
void Recv_Data_Handle(u8 *data,u16 num);
void Send_Live_Temp_Message(u32 time,u16 in_temp,u16 probe1_temp,u16 probe2_temp);		//发送实时温度
void Send_Goto_WiFi_Password_Mode(void);
void Send_ALL_Set_Temp_Message(u16 in_temp,u16 probe1_temp);		//发送设定温度
void Send_Run_Time_Message(u32 time);		//发送运行时间
void Wifi_Init(void);
void Send_Recipe_Message(void);
void Send_Recipe_Over_Message(void);
void Send_Model_Message(void);
void Send_Off_Temp(u8 r_w,u16 off_temp,u16 off_time);
void Send_PID_Message(u8 feedback,u8 app_kp,u8 app_ki,u8 app_kd,u16 app_fan_pwm);


#endif




