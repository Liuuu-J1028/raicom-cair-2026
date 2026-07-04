#include "Gpio.h"
#include "Delay.h"

void  Gpio_Init(void) {
	 /**********************
	 1.执行端口重映射时,复用功能时钟得使能:RCC_APB2Periph_AFIO
	 
	 2.  &1.GPIO_Remap_SWJ_Disable: !< Full SWJ Disabled (JTAG-DP + SW-DP)
		  此时PA13|PA14|PA15|PB3|PB4都可作为普通IO用了
		为了保存某些调试端口,GPIO_Remap_SWJ_Disable也可选择为下面两种模式：
	  
		 &2.GPIO_Remap_SWJ_JTAGDisable: !< JTAG-DP Disabled and SW-DP Enabled
		 此时PA15|PB3|PB4可作为普通IO用了
	  
		 &3.GPIO_Remap_SWJ_NoJTRST: !< Full SWJ Enabled (JTAG-DP + SW-DP) but without JTRST
		 此时只有PB4可作为普通IO用了 
	 **********************/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE); 		//使能 PA 端口时钟
	//GPIO_PinRemapConfig(GPIO_Remap_SWJ_Disable, ENABLE);  //使能禁止JTAG
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);  //使能禁止JTAG	
	return;	
}

void Led1_Init() {
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE); //使能 PB 端口时钟
	
	//nled
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13; //配置 pin10
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; //推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; //IO 翻转 50MHz
	GPIO_Init(GPIOB, &GPIO_InitStructure);
		
}

void Led1_Switch(void) {
	static u8 flag = 0;
	if(flag) {
		Led_On();
	} else {
		Led_Off();
	}
	flag = ~flag;
}

void Beep_Init(void) {
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB , ENABLE); //使能 PB 端口时钟
	
	//beep
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5; //配置 pin5
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; //IO 翻转 50MHz
	GPIO_Init(GPIOB, &GPIO_InitStructure);
}

void Beep_Times(u32 time, u32 count) {
	int i;
	for(i=0;i<count;i++) {
		Beep_On();Led_On();
		Delay_ms(time);
		Beep_Off();Led_Off();
		Delay_ms(time);
	}
}

void GpioA_PIN_Set(unsigned char pin, unsigned char level) {
	if(level) {
		GPIO_SetBits(GPIOA,1 << pin);
	} else {
		GPIO_ResetBits(GPIOA,1 << pin);
	}
}

void GpioB_PIN_Set(unsigned char pin, unsigned char level) {
	if(level) {
		GPIO_SetBits(GPIOB,1 << pin);
	} else {
		GPIO_ResetBits(GPIOB,1 << pin);
	}
}


void GpioC_PIN_Set(unsigned char pin, unsigned char level) {
	if(level) {
		GPIO_SetBits(GPIOC,1 << pin);
	} else {
		GPIO_ResetBits(GPIOC,1 << pin);
	}
}

void PWM_Servos_Init(void) {
	GPIO_InitTypeDef GPIO_InitStructure;	

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOB, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_8|GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_3|GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_6|GPIO_Pin_7|GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);	
}

void Servos_IO_Set(u8 index, u8 level) {
	switch(index) {
		case 0:GpioB_PIN_Set(3, level);break;
		case 1:GpioB_PIN_Set(8, level);break;
		case 2:GpioB_PIN_Set(9, level);break;
		case 3:GpioB_PIN_Set(6, level);break;
		case 4:GpioB_PIN_Set(7, level);break;
		case 5:GpioB_PIN_Set(4, level);break;
		default:break;
	}
}

void sensor_init(void) {
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);  
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;   //PB11
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;   
	GPIO_Init(GPIOB, &GPIO_InitStructure); 
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;   //PA1
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;   
	GPIO_Init(GPIOA, &GPIO_InitStructure); 
}

/***********************************************
	函数名称：Beep_On_times() 
    功能介绍：控制蜂鸣器的响
	函数参数：times-响的次数，delay-间隔时间
	返回值：  无
 ***********************************************/
void Beep_On_times(int times, int delay) {
	int i;
	for(i=0;i<times;i++) {
		Beep_On();
		Delay_ms(delay);
		Beep_Off();
		Delay_ms(delay);
	}
}
