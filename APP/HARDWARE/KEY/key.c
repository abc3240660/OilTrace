#include "key.h"
#include "stm32f10x_exti.h"
#include "common.h"

u16 g_pc6_cnt = 0;
u16 g_pc7_cnt = 0;
u16 g_pc8_cnt = 0;
u16 g_pc9_cnt = 0;
u16 g_pc10_cnt = 0;
u16 g_pc11_cnt = 0;

u8 exit_twice_calc_flag[6] = {0};

u16 time_start_pc6 = 0;
u16 time_start_pc7 = 0;
u16 time_start_pc8 = 0;
u16 time_start_pc9 = 0;
u16 time_start_pc10 = 0;
u16 time_start_pc11 = 0;

extern u32 os_jiffies_10ms;

//������ʼ������
void EC11KEY_Init(void) //IO��ʼ��
{ 
 	GPIO_InitTypeDef GPIO_InitStructure;
 
 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC,ENABLE);//ʹ��PORTA,PORTEʱ��

	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_6|GPIO_Pin_7|GPIO_Pin_8;//KEY0-KEY1
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; //���ó���������
 	GPIO_Init(GPIOC, &GPIO_InitStructure);//��ʼ��GPIOE4,3
	
	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_9|GPIO_Pin_10|GPIO_Pin_11;//KEY0-KEY1
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; //���ó���������
 	GPIO_Init(GPIOC, &GPIO_InitStructure);//��ʼ��GPIOE4,3
}

void EC11_EXTI_Init(void)
{
   	EXTI_InitTypeDef EXTI_InitStructure;
 	  NVIC_InitTypeDef NVIC_InitStructure;

    EC11KEY_Init();	 //	�����˿ڳ�ʼ��

  	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);	//ʹ�ܸ��ù���ʱ��

    //GPIOC6
  	GPIO_EXTILineConfig(GPIO_PortSourceGPIOC,GPIO_PinSource6);
  	EXTI_InitStructure.EXTI_Line=EXTI_Line6;
  	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;	
  	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;//EXTI_Trigger_Falling��EXTI_Trigger_Rising_Falling
		EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  	EXTI_Init(&EXTI_InitStructure);	  	//����EXTI_InitStruct��ָ���Ĳ�����ʼ������EXTI�Ĵ���

		NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
  	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;			//ʹ�ܰ���KEY1���ڵ��ⲿ�ж�ͨ��
  	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x00;	//��ռ���ȼ�2 
  	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x00;					//�����ȼ�1 
  	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;								//ʹ���ⲿ�ж�ͨ��
  	NVIC_Init(&NVIC_InitStructure);  	  //����NVIC_InitStruct��ָ���Ĳ�����ʼ������NVIC�Ĵ���
		
    //GPIOC7
  	GPIO_EXTILineConfig(GPIO_PortSourceGPIOC,GPIO_PinSource7);
  	EXTI_InitStructure.EXTI_Line=EXTI_Line7;
  	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;	
  	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;//EXTI_Trigger_Falling��EXTI_Trigger_Rising_Falling
		EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  	EXTI_Init(&EXTI_InitStructure);	  	//����EXTI_InitStruct��ָ���Ĳ�����ʼ������EXTI�Ĵ���

		NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
  	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;			//ʹ�ܰ���KEY1���ڵ��ⲿ�ж�ͨ��
  	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x00;	//��ռ���ȼ�2 
  	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x00;					//�����ȼ�1 
  	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;								//ʹ���ⲿ�ж�ͨ��
  	NVIC_Init(&NVIC_InitStructure);  	  //����NVIC_InitStruct��ָ���Ĳ�����ʼ������NVIC�Ĵ���
		
    //GPIOC8
  	GPIO_EXTILineConfig(GPIO_PortSourceGPIOC,GPIO_PinSource8);
  	EXTI_InitStructure.EXTI_Line=EXTI_Line8;
  	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;	
  	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;//EXTI_Trigger_Falling��EXTI_Trigger_Rising_Falling
		EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  	EXTI_Init(&EXTI_InitStructure);	  	//����EXTI_InitStruct��ָ���Ĳ�����ʼ������EXTI�Ĵ���

		NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
  	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;			//ʹ�ܰ���KEY1���ڵ��ⲿ�ж�ͨ��
  	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x00;	//��ռ���ȼ�2 
  	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x00;					//�����ȼ�1 
  	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;								//ʹ���ⲿ�ж�ͨ��
  	NVIC_Init(&NVIC_InitStructure);  	  //����NVIC_InitStruct��ָ���Ĳ�����ʼ������NVIC�Ĵ���
		
    //GPIOC9
  	GPIO_EXTILineConfig(GPIO_PortSourceGPIOC,GPIO_PinSource9);
  	EXTI_InitStructure.EXTI_Line=EXTI_Line9;
  	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;	
  	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;//EXTI_Trigger_Falling��EXTI_Trigger_Rising_Falling
		EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  	EXTI_Init(&EXTI_InitStructure);	  	//����EXTI_InitStruct��ָ���Ĳ�����ʼ������EXTI�Ĵ���

		NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
  	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;			//ʹ�ܰ���KEY1���ڵ��ⲿ�ж�ͨ��
  	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x00;	//��ռ���ȼ�2 
  	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x00;					//�����ȼ�1 
  	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;								//ʹ���ⲿ�ж�ͨ��
  	NVIC_Init(&NVIC_InitStructure);  	  //����NVIC_InitStruct��ָ���Ĳ�����ʼ������NVIC�Ĵ���
		
    //GPIOC10
  	GPIO_EXTILineConfig(GPIO_PortSourceGPIOC,GPIO_PinSource10);
  	EXTI_InitStructure.EXTI_Line=EXTI_Line10;
  	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;	
  	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;//EXTI_Trigger_Falling��EXTI_Trigger_Rising_Falling
		EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  	EXTI_Init(&EXTI_InitStructure);	  	//����EXTI_InitStruct��ָ���Ĳ�����ʼ������EXTI�Ĵ���

		NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
  	NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;			//ʹ�ܰ���KEY1���ڵ��ⲿ�ж�ͨ��
  	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x00;	//��ռ���ȼ�2 
  	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x00;					//�����ȼ�1 
  	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;								//ʹ���ⲿ�ж�ͨ��
  	NVIC_Init(&NVIC_InitStructure);  	  //����NVIC_InitStruct��ָ���Ĳ�����ʼ������NVIC�Ĵ���
		
    //GPIOC11
  	GPIO_EXTILineConfig(GPIO_PortSourceGPIOC,GPIO_PinSource11);
  	EXTI_InitStructure.EXTI_Line=EXTI_Line11;
  	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;	
  	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;//EXTI_Trigger_Falling��EXTI_Trigger_Rising_Falling
		EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  	EXTI_Init(&EXTI_InitStructure);	  	//����EXTI_InitStruct��ָ���Ĳ�����ʼ������EXTI�Ĵ���

		NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
  	NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;			//ʹ�ܰ���KEY1���ڵ��ⲿ�ж�ͨ��
  	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x00;	//��ռ���ȼ�2 
  	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x00;					//�����ȼ�1 
  	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;								//ʹ���ⲿ�ж�ͨ��
  	NVIC_Init(&NVIC_InitStructure);  	  //����NVIC_InitStruct��ָ���Ĳ�����ʼ������NVIC�Ĵ���
}

u16 calc_count_gap_10ms(u16 time_start, u16 time_end)
{
	if (time_end >= time_start) {
		if (time_end == time_start) {
			return 10;// to forbiden x/0 -> 60000 / (10*10) = 600
		} else {
			return (time_end-time_start);
		}
	} else {
		return (10000-time_start+time_end);
	}
}

//�ⲿ�ж�3�������
void EXTI9_5_IRQHandler(void)
{	
	if (EXTI_GetITStatus(EXTI_Line6) != RESET) {
		if (0 == exit_twice_calc_flag[0]) {
			time_start_pc6 = os_jiffies_10ms;
			exit_twice_calc_flag[0] = 1;
		} else {
			g_pc6_cnt = 60000 / (calc_count_gap_10ms(time_start_pc6, os_jiffies_10ms)*10);
			time_start_pc6 = os_jiffies_10ms;
		}

		// exit_twice_calc_flag[0] = !exit_twice_calc_flag[0];

		printf("EXTI_Line6, os_jiffies_10ms = %d, g_pc6_cnt = %d\n", os_jiffies_10ms, g_pc6_cnt);
		EXTI_ClearITPendingBit(EXTI_Line6);  //���LINE3�ϵ��жϱ�־λ  
	}
	if (EXTI_GetITStatus(EXTI_Line7) != RESET) {
		if (0 == exit_twice_calc_flag[1]) {
			time_start_pc7 = os_jiffies_10ms;
			exit_twice_calc_flag[1] = 1;
		} else {
			g_pc7_cnt = 60000 / (calc_count_gap_10ms(time_start_pc7, os_jiffies_10ms)*10);
			time_start_pc7 = os_jiffies_10ms;
		}

		// exit_twice_calc_flag[1] = !exit_twice_calc_flag[1];

		printf("EXTI_Line7, os_jiffies_10ms = %d, g_pc7_cnt = %d\n", os_jiffies_10ms, g_pc7_cnt);
		EXTI_ClearITPendingBit(EXTI_Line7);  //���LINE3�ϵ��жϱ�־λ  
	}
	if (EXTI_GetITStatus(EXTI_Line8) != RESET) {
		if (0 == exit_twice_calc_flag[2]) {
			time_start_pc8 = os_jiffies_10ms;
			exit_twice_calc_flag[2] = 1;
		} else {
			g_pc8_cnt = 60000 / (calc_count_gap_10ms(time_start_pc8, os_jiffies_10ms)*10);
			time_start_pc8 = os_jiffies_10ms;
		}

		// exit_twice_calc_flag[2] = !exit_twice_calc_flag[2];

		printf("EXTI_Line8, os_jiffies_10ms = %d, g_pc8_cnt = %d\n", os_jiffies_10ms, g_pc8_cnt);
		EXTI_ClearITPendingBit(EXTI_Line8);  //���LINE3�ϵ��жϱ�־λ  
	}
	if (EXTI_GetITStatus(EXTI_Line9) != RESET) {
		if (0 == exit_twice_calc_flag[3]) {
			time_start_pc9 = os_jiffies_10ms;
			exit_twice_calc_flag[3] = 1;
		} else {
			g_pc9_cnt = 60000 / (calc_count_gap_10ms(time_start_pc9, os_jiffies_10ms)*10);
			time_start_pc9 = os_jiffies_10ms;
		}

		// exit_twice_calc_flag[3] = !exit_twice_calc_flag[3];

		printf("EXTI_Line9, os_jiffies_10ms = %d, g_pc9_cnt = %d\n", os_jiffies_10ms, g_pc9_cnt);
		EXTI_ClearITPendingBit(EXTI_Line9);  //���LINE3�ϵ��жϱ�־λ  
	}
}

void EXTI15_10_IRQHandler(void)
{	
	if (EXTI_GetITStatus(EXTI_Line10) != RESET) {
		if (0 == exit_twice_calc_flag[4]) {
			time_start_pc10 = os_jiffies_10ms;
			exit_twice_calc_flag[4] = 1;
		} else {
			g_pc10_cnt = 60000 / (calc_count_gap_10ms(time_start_pc10, os_jiffies_10ms)*10);
			time_start_pc10 = os_jiffies_10ms;
		}

		// exit_twice_calc_flag[4] = !exit_twice_calc_flag[4];

		printf("EXTI_Line10, os_jiffies_10ms = %d, g_pc10_cnt = %d\n", os_jiffies_10ms, g_pc10_cnt);
		EXTI_ClearITPendingBit(EXTI_Line10);  //���LINE3�ϵ��жϱ�־λ  
	}
	if (EXTI_GetITStatus(EXTI_Line11) != RESET) {
		if (0 == exit_twice_calc_flag[5]) {
			time_start_pc11 = os_jiffies_10ms;
			exit_twice_calc_flag[5] = 1;
		} else {
			g_pc11_cnt = 60000 / (calc_count_gap_10ms(time_start_pc11, os_jiffies_10ms)*10);
			time_start_pc11 = os_jiffies_10ms;
		}

		// exit_twice_calc_flag[5] = !exit_twice_calc_flag[5];

		printf("EXTI_Line11, os_jiffies_10ms = %d, g_pc11_cnt = %d\n", os_jiffies_10ms, g_pc11_cnt);
		EXTI_ClearITPendingBit(EXTI_Line11);  //���LINE3�ϵ��жϱ�־λ  
	}
}

/***************************END OF FILE***************************/
