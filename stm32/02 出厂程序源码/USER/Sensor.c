/*传感器功能代码*/

#include "Rcc.h"		//配置时钟文件
#include "Gpio.h"		//配置IO口文件
#include "Global.h"	    //存放全局变量
#include "Delay.h"	    //存放延时函数
#include "Type.h"		//存放类型定义
#include "Usart.h"	    //存放串口功能文件
#include "Timer.h"	    //存放定时器功能文件
#include "PS2.h"		//存放索尼手柄
#include "W25q64.h"	    //存储芯片的操作
#include "Color.h"      //颜色识别文件
#include <stdio.h>		//标准库文件
#include <string.h>		//标准库文件
#include <math.h>		//标准库文件
#include "Kinematics.h"	//逆运动学算法
#include "stm32f10x_iwdg.h"
#include "Sensor.h"
#include "Action.h"
#include "Main.h"

COLOR_RGBC color_rgbc;

u16 kms_y = 0;
u8 get_count = 0;
u8 carry_step = 0;
/*  GPIO口分布
	双路循迹传感器 S1  S2
	Signal1 - PA0  Signal2 - PA1
	超声波传感器   S3  
    TRIG - PB0  Echo - PA2      */

/*************************************************************
函数名称：Loop_Sonser()
功能介绍：检测智能功能指令，改变模式
函数参数：无
返回值：  无  
*************************************************************/
void Loop_Sonser(void){
	if(Auto_Mode == 1){
		Distance_Following();
	}else if (Auto_Mode == 2){
		Obstacle_Avoidance();
	}else if (Auto_Mode == 3){
		Auto_Tracking();
	}else if (Auto_Mode == 4){
		Tracking_Avoidance();
	}
}


/*初始化传感器IO口*/
void Sensor_Init(void) {
	Tracking_Gpio_Init();
	Ultrasonic_Gpio_Init();
    TCS34725_Init(TCS34725_INTEGRATIONTIME_24MS);
}

/*************************************************************
函数名称：Ultrasonic_Gpio_Init()
功能介绍：初始化超声波传感器IO口
函数参数：无
返回值：  无  
*************************************************************/
void Ultrasonic_Gpio_Init(void){
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOA, ENABLE);  
	
	//初始化超声波IO口 Trig PB0  Echo PA2
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	
	GPIO_Init(GPIOB, &GPIO_InitStructure); 	
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;   
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;  
	GPIO_Init(GPIOA, &GPIO_InitStructure); 	
	
	//初始化超声波定时器
	TIM3_Int_Init(30000, 71);
}

/*************************************************************
函数名称：Tracking_Gpio_Init()
功能介绍：初始化循迹传感器IO口
函数参数：无
返回值：  无  
*************************************************************/
void Tracking_Gpio_Init(void) {
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);  
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1; 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;   
	GPIO_Init(GPIOA, &GPIO_InitStructure); 	
}

void Ultrasonic_Delay_us(uint16_t time)  //延时函数
{ 
	uint16_t i,j;
	for(i=0;i<time;i++)
  		for(j=0;j<9;j++);
}

/*************************************************************
函数名称：Ultrasonic_Get_Data()
功能介绍：采集超声波数据
函数参数：无
返回值：  采集的数据  
*************************************************************/
int Ultrasonic_Get_Data(void) {
	u16 csb_t;
	long timeout= 0;
	Trig(1);
	Ultrasonic_Delay_us(30);
	Trig(0);
	timeout= 0;
	while(Echo() == 0&&(timeout++<10000)) //等待接收口高电平输出
	if(timeout>=500000)return 0;
	TIM_SetCounter(TIM3,0);//清除计数
	TIM_Cmd(TIM3, ENABLE);  //使能TIMx外设
	while(Echo() == 1&&(timeout++<10000))
	if(timeout>=500000)return 0;
	TIM_Cmd(TIM3, DISABLE);  //使能TIMx外设      
	csb_t = TIM_GetCounter(TIM3);//获取时间,分辨率为1US
	//340m/s = 0.017cm/us
	if(csb_t < 25000) {
		//sprintf((char *)cmd_return, "csb_time=%d\r\n", (int)(csb_t*0.17));
		//Usart1_Send_Str(cmd_return);
		csb_t = csb_t*0.017;
		return csb_t;
	}
	return 0;
}

/*************************************************************
函数名称：Ultrasonic_Data_Handle()
功能介绍：处理超声波采集到的数据，取采集到的中间值
函数参数：无
返回值：  处理采集到的超声波数据  
*************************************************************/
int Ultrasonic_Data_Handle(void) {
	u8 i;
	static int ad_value[5] = {0}, myvalue;// ad_value_bak[5] = {0}, 
	for(i=0;i<5;i++)ad_value[i] = Ultrasonic_Get_Data();
	selection_sort(ad_value, 5);
	myvalue = ad_value[2];
// 	for(i=0;i<5;i++)ad_value[i] = ad_value_bak[i];
	return myvalue;  
}


/*************************************************************
函数名称：Distance_Following() 定距跟随
功能介绍：判断超声波检测的距离，小于20cm时后退；25-35cm或超过70cm时停止；40-60cm时前进
函数参数：无
返回值：  无
*************************************************************/
void Distance_Following(void){
	static u32 systick_ms_bak = 0;
	int adc_csb;
	if(millis() - systick_ms_bak > 100) {
		systick_ms_bak = millis();
		adc_csb = Ultrasonic_Data_Handle();             //获取a0的ad值，计算出距离
		if(adc_csb < 20) {                          //距离小于20cm时后退
			Motors_Run(-600,-600,-600,-600);
			Delay_ms(300);
		} else if ((25 < adc_csb && adc_csb < 35) || adc_csb > 70) {   //25-35cm或超过70cm时停止
			Motors_Run(0,0,0,0);
			Delay_ms(300);
		} else if (40 < adc_csb && adc_csb < 60) {  //40-60cm时前进
			Motors_Run(600,600,600,600);
			Delay_ms(300);
		}
	}
}


/*************************************************************
函数名称：Obstacle_Avoidance() 自由避障
功能介绍：判断超声波检测的距离，小于20cm时右转避障，大于20cm时前进
函数参数：无
返回值：  无
*************************************************************/
void Obstacle_Avoidance(void){
	static u32 systick_ms_bak = 0;
	int adc_csb;
	if(millis() - systick_ms_bak > 100) {
		systick_ms_bak = millis();
		adc_csb = Ultrasonic_Data_Handle();//获取a0的ad值，计算出距离
		if(adc_csb < 20) {//距离低于20cm就右转
			Motors_Run(600,-600,600,-600);
			Delay_ms(300);
		} else {
			Motors_Run(600,600,600,600);
		}
	}
}


/*************************************************************
函数名称：Auto_Tracking() 智能循迹
功能介绍：循迹传感器探测部位在黑线上时返回0，不在线上返回1
函数参数：无
返回值：  无
*************************************************************/
void Auto_Tracking(void){
	static u32 systick_ms_bak = 0;	
	if(millis() - systick_ms_bak > 50){
		systick_ms_bak = millis();
		if(Sensor_Tracking_Left() == 1 && Sensor_Tracking_Right() == 1){
			Motors_Run(Tracking_Speed,Tracking_Speed,Tracking_Speed,Tracking_Speed);		
		}else if (Sensor_Tracking_Left() == 0 && Sensor_Tracking_Right() == 1){
			Motors_Run(Tracking_Speed,0,Tracking_Speed,0);		
		}else if (Sensor_Tracking_Left() == 1 && Sensor_Tracking_Right() == 0){
			Motors_Run(0,Tracking_Speed,0,Tracking_Speed);		
		}else if (Sensor_Tracking_Left() == 0 && Sensor_Tracking_Right() == 0){
			Motors_Run(0,0,0,0);
        }
	}
}

/*************************************************************
函数名称：Tracking_Avoidance() 循迹避障
功能介绍：循迹避障：前方20cm内没有障碍物时，循迹；前方20cm内有障碍物时，停止
函数参数：无
返回值：  无
*************************************************************/
void Tracking_Avoidance(void){
	static u32 systick_ms_bak = 0;
	int adc_csb;
	if(millis() - systick_ms_bak > 50) {
		systick_ms_bak = millis();
		adc_csb = Ultrasonic_Data_Handle();//获取a0的ad值，计算出距离
		if(adc_csb < 20) {//距离低于20cm就停止
			Motors_Run(0,0,0,0);
			Delay_ms(500);
		} else {          //否则循迹
			if(Sensor_Tracking_Left() == 1 && Sensor_Tracking_Right() == 1){
				Motors_Run(Tracking_Speed,Tracking_Speed,Tracking_Speed,Tracking_Speed);		
			}else if (Sensor_Tracking_Left() == 0 && Sensor_Tracking_Right() == 1){
				Motors_Run(0,Tracking_Speed,0,Tracking_Speed);		
			}else if (Sensor_Tracking_Left() == 1 && Sensor_Tracking_Right() == 0){
				Motors_Run(Tracking_Speed,0,Tracking_Speed,0);		
			}else if (Sensor_Tracking_Left() == 0 && Sensor_Tracking_Right() == 0){
				Motors_Run(0,0,0,0);		
			}
		}
	}
}
/*************************************************************
功能介绍：识别木块颜色，夹取分别放到不同位置
函数参数：无
返回值：  无  
******************************************************** *****/
void Color_Sorting(void) {
	static u32 systick_ms_ColorSorting = 0;     //静态变量，每执行一次颜色分拣，时间更新一次
    static u8 Sorting_Mode = 0;                 //静态变量，作为执行动作标志位
    if (Sorting_Mode == 0 && millis() - systick_ms_ColorSorting > 50 && Group_Do_Complete == 1) {
        systick_ms_ColorSorting = millis();
		TCS34725_GetRawData(&color_rgbc);   //调用函数获取RGB的值
    	TCS34725_LedON(0);                  //关闭颜色识别传感器的LED灯
        if (color_rgbc.c < 1) {       
			TCS34725_LedON(1);              //打开颜色识别传感器的LED灯
			Delay_ms(800);
			TCS34725_GetRawData(&color_rgbc);//调用函数获取RGB的值
            sprintf((char *)cmd_return, "R=%d,G=%d,B=%d,C=%d\r\n",color_rgbc.r,color_rgbc.g,color_rgbc.b,color_rgbc.c);
            Usart1_Send_Str(cmd_return);    //打印输出R、G、B、C的值
			if(color_rgbc.r > color_rgbc.g && color_rgbc.r  > color_rgbc.b ) {					
					Sorting_Mode = 1;
                    Usart1_Send_Str((u8 *)"RED\r\n");
			} else if (color_rgbc.g > color_rgbc.r && color_rgbc.g  > color_rgbc.b) {     
					Sorting_Mode = 2;
                    Usart1_Send_Str((u8 *)"GREEN\r\n");
			} else if (color_rgbc.b > color_rgbc.g && color_rgbc.b  > color_rgbc.r) {       
					Sorting_Mode = 3;
                    Usart1_Send_Str((u8 *)"BLUE\r\n");
			}			
		}
	}
    if(Sorting_Mode){
        if(Sorting_Mode == 1){
            Parse_Group_CMD((u8*)"$DGT:25-33,1!"); //执行脱机存储动作组前放				
        }else if(Sorting_Mode == 2){
            Parse_Group_CMD((u8*)"$DGT:34-42,1!"); //执行脱机存储动作组左放			
        }else if(Sorting_Mode == 3){
            Parse_Group_CMD((u8*)"$DGT:43-51,1!"); //执行脱机存储动作组左放			
        }
            Sorting_Mode = 0;
    }
}

