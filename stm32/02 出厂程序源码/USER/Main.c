/***************************************************************
	*	@笔者	：	TWKJ
	*	@日期	：	2023年09月21日
	*	@所属	：	苏州滔为智能科技有限公司
	*	@官网	：	http://www.80topway.com
	*	@功能	：	启星STM32控制板
***************************************************************/
/***************************************************************
                    精灵搬运小车出厂程序
	实现的功能：
	1、功能：
        前进、后退、左转、右转、左平移、右平移、停止
        定距跟随、自由避障、智能循迹、循迹避障
        机械臂各关节舵机的控制
	2、控制方式：
        基本控制方式   ：手柄、微信小程序、PC上位机
        可拓展控制方式 ：语音识别控制
	
    各个引脚配置情况：
    1、传感器引脚:
		循迹（S1-PA0 PA1） 
		超声波(S3-PB0 PA2) 
	2、舵机引脚：
		DJ0-PB3
		DJ1-PB8
		DJ2-PB9
		DJ3-PB6
		DJ4-PB7
		DJ5-PB4
	3、蜂鸣器引脚：
		BEEP-PB5
	4、LED引脚：
		NLED-PB13
    5、PS2手柄引脚：	
        PS1-DAT-PA15
        PS2-CMD-PA14
        PS6-ATT-PA13
        PS7-CLK-PA12
	6、按键引脚：
        KEY1-PA8 KEY2-PA11
	7、统一总线口： TXD3 RXD3
	以上引脚均可在我们提供的原理图内找到。
    
	主频：72M
	单片机型号：STM32F103C8T6
	
***************************************************************/
#include "stm32f10x.h"      // Device header
#include "stm32f10x_iwdg.h"

#include "Rcc.h"	    	//配置时钟文件
#include "Gpio.h"		    //配置IO口文件
#include "Global.h"	        //存放全局变量
#include "Delay.h"	        //存放延时函数
#include "Type.h"		    //存放类型定义
#include "Usart.h"	        //存放串口功能文件
#include "Timer.h"	        //存放定时器功能文件
#include "PS2.h"		    //存放索尼手柄
#include "W25q64.h"	        //存储芯片的操作
#include <stdio.h>	    	//标准库文件
#include <string.h>	    	//标准库文件
#include <math.h>		    //标准库文件
#include "Kinematics.h"	    //逆运动学算法
#include "Action.h"         //动作组执行文件
#include "Sensor.h"         //传感器相关文件

#define MODULE "TOPWAY-Qixing32"

/*全局变量定义*/
u8 PS2_Buf[9]={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};   //存储手柄的数据
u8 Voice_Flag = 0;  //用于控制语音识别时小车的基本动作执行时间为2秒
u8 Auto_Mode = 0;   //定义模式参数
#define SYS_MODE_REMOTE  0
#define SYS_MODE_VISION  1
u8 Sys_Mode = SYS_MODE_REMOTE;  //系统模式：0=遥控模式, 1=视觉自主模式
u8 rx_cmd = 0;                  //USART2接收到的视觉指令
/*             参数说明：
	Auto_Mode=1时，开启定距跟随功能
	Auto_Mode=2时，开启自由避障功能
	Auto_Mode=3时，开启智能循迹功能
	Auto_Mode=4时，开启循迹避障功能     */
int Tracking_Speed = 350;       //循迹速度,范围（0-1000），可根据实际情况修改循迹速度

kinematics_t kinematics;

/*PS2手柄按键的配置（红灯模式）*/
const char *pre_cmd_set_red[PSX_BUTTON_NUM] = { //红灯模式下按键的配置			
	"<PS2_RED01:#005P0600T2000!^#005PDST!>",	//L2						  
	"<PS2_RED02:#005P2400T2000!^#005PDST!>",	//R2						  
	"<PS2_RED03:#004P2400T2000!^#004PDST!>",	//L1						  
	"<PS2_RED04:#004P0600T2000!^#004PDST!>",	//R1			
	"<PS2_RED05:#002P2400T2000!^#002PDST!>",	//RU						  
	"<PS2_RED06:#003P2400T2000!^#003PDST!>",	//RR						  
	"<PS2_RED07:#002P0600T2000!^#002PDST!>",	//RD						  
	"<PS2_RED08:#003P0600T2000!^#003PDST!>",	//RL				
	"<PS2_RED09:$MODE!>",					    //SE    				  
	"<PS2_RED10:>",					            //AL						   
	"<PS2_RED11:>",					            //AR						  
	"<PS2_RED12:$DJR!>",					    //ST		
	"<PS2_RED13:#001P0600T2000!^#001PDST!>",	//LU						  
	"<PS2_RED14:#000P0600T2000!^#000PDST!>",	//LR								  
	"<PS2_RED15:#001P2400T2000!^#001PDST!>",	//LD						  
	"<PS2_RED16:#000P2400T2000!^#000PDST!>",	//LL								
};

//绿灯模式下，摇杆会映射到按键，防止影响，去掉绿灯模式功能
/*PS2手柄按键的配置（红灯模式）*/
/*
const char *pre_cmd_set_grn[PSX_BUTTON_NUM] = {//绿灯模式下按键的配置			
	"<PS2_GRN01:#005P0600T2000!^#005PDST!>",	 //L2						  
	"<PS2_GRN02:#005P2400T2000!^#005PDST!>",	 //R2						  
	"<PS2_GRN03:#004P0600T2000!^#004PDST!>",	 //L1						  
	"<PS2_GRN04:#004P2400T2000!^#004PDST!>",	 //R1			
	"<PS2_GRN05:#002P2400T2000!^#002PDST!>",	 //RU						  
	"<PS2_GRN06:#003P2400T2000!^#003PDST!>",	 //RR						  
	"<PS2_GRN07:#002P0600T2000!^#002PDST!>",	 //RD						  
	"<PS2_GRN08:#003P0600T2000!^#003PDST!>",	 //RL				
	"<PS2_GRN09:$DJR!>",					             //SE    				  
	"<PS2_GRN10:>",					                   //AL						   
	"<PS2_GRN11:>",					                   //AR						  
	"<PS2_GRN12:$DJR!>",					             //ST		
	"<PS2_GRN13:#001P0600T2000!^#001PDST!>",	 //LU						  
	"<PS2_GRN14:#000P0600T2000!^#000PDST!>",	 //LR								  
	"<PS2_GRN15:#001P2400T2000!^#001PDST!>",	 //LD						  
	"<PS2_GRN16:#000P2400T2000!^#000PDST!>",	 //LL								
};
*/

/*-------------------------------------------------------------------------------------------------------
*  程序从这里执行				
*  这个启动代码 完成时钟配置 使用外部晶振作为STM32的运行时钟 并倍频到72M
-------------------------------------------------------------------------------------------------------*/


int main(void) {	
	Rcc_Init();		            //初始化时钟
	Global_Init();		        //初始化全局变量
	Gpio_Init();		        //初始化IO口
	Led1_Init();		        //初始化工作指示灯
    Beep_Init();		        //初始化蜂鸣器
	PWM_Servos_Init();          //初始化PWM舵机IO口
	W25Q64_Init();		        //初始化存储器W25Q64
	PSX_init();			        //初始化PS2手柄
	Usart1_Init(115200);        //初始化串口1 用于下载动作组
    Usart3_Init(115200);        //初始化串口3 用于底板总线、蓝牙、lora
	SysTick_Int_Init();	        //初始化滴答时钟，1S增加一次millis()的值
	TIM2_Int_Init(20000, 71);   //初始化定时器2，处理舵机PWM输出	
	Servos_Bias_Init();         //初始化舵机，将偏差代入初始值
	IWDG_Init();                //初始化独立看门狗    
    Sensor_Init();              //初始化传感器
    Delay_Init();               //初始化延时函数
    
    Beep_Off();                 //关闭蜂鸣器
    Led_Off();	                //关闭工作指示灯
    Usart1_Open();              //打开串口1	
    Usart3_Open();              //打开串口3
    Interrupt_Open();           //初始化总中断，并打开
    setup_kinematics(110, 105, 75, 190, &kinematics); //kinematics 90mm 105mm 98mm 150mm

   	Starting_up();		        //开机启动信号,蜂鸣器响三下，指示灯闪烁三下	
    Starting_Group();           //开机动作，机械臂呈蜷缩状态
	
    
	while(1) {

		Loop_Led1();		    //循环执行工作指示灯，500ms跳动一次
		Loop_Usart();	    	//串口数据接收处理

		if(Sys_Mode == SYS_MODE_REMOTE) {
			// ========== 遥控模式 ==========
			// 原有功能全部保留：PS2手柄、传感器自动模式、动作组
			Loop_Actions();	        //动作组批量执行
			Loop_PS2_Data();        //循环读取PS2手柄数据
			Loop_PS2_Buttons();     //处理手柄上的按钮
			Loop_PS2_Motors();      //处理手柄摇杆数据，控制电机转动
			Loop_Sonser();		    //传感器自动模式（循迹/避障等）
			// Loop_ColorSorting(); //处理颜色识别结果并执行动作组(若要使用此功能，取消注释即可)
		} else {
			// ========== 视觉自主模式 ==========
			// 完全隔离：只处理ESP32视觉指令，不响应PS2摇杆/按键
			Loop_Vision_Auto();
		}

    }
}

/***********************************************
	函数名称：W25Q64_Init() 
	功能介绍：初始化存储器W25Q64
	函数参数：无
	返回值：  无
 ***********************************************/
void W25Q64_Init(void) { 
	u8 i;
	spiFlahsOn(1);
	w25x_init();	        //动作组存储芯片初始化
	w25x_read((u8 *)(&eeprom_info), W25Q64_INFO_ADDR_SAVE_STR, sizeof(eeprom_info));//读取全局变量
	
	if(eeprom_info.version != VERSION) {    //判断版本是否是当前版本
		eeprom_info.version = VERSION;		//复制当前版本
		eeprom_info.dj_record_num = 0;		//学习动作组变量赋值0
	}
	
	if(eeprom_info.dj_bias_pwm[Servos_NUM] != FLAG_VERIFY) {
		for(i=0;i<Servos_NUM;i++) {
			eeprom_info.dj_bias_pwm[i] = 0;
		}
		eeprom_info.dj_bias_pwm[Servos_NUM] = FLAG_VERIFY;
	}
	
	for(i=0;i<Servos_NUM;i++) {
		Servos_Doing[i].aim = 1500 + eeprom_info.dj_bias_pwm[i];
		Servos_Doing[i].cur = 1500 + eeprom_info.dj_bias_pwm[i];
		Servos_Doing[i].inc = 0;
	}
	spiFlahsOn(0);
}	

/***********************************************
	函数名称：Starting_up() 
    功能介绍：系统启动时蜂鸣器鸣叫3声，LED灯闪烁3下，示意启动
	函数参数：无
	返回值：  无
 ***********************************************/
void Starting_up(void) {
	//蜂鸣器LED 名叫闪烁 示意系统启动
	Beep_On();Led_On();Delay_ms(100);
    Beep_Off();Led_Off();Delay_ms(100);
	Beep_On();Led_On();Delay_ms(100);
    Beep_Off();Led_Off();Delay_ms(100);
	Beep_On();Led_On();Delay_ms(100);
    Beep_Off();Led_Off();Delay_ms(100);
}	


/***********************************************
	函数名称：Loop_Led1() 
    功能介绍：循环执行工作指示灯，1000ms跳动一次
	函数参数：无
	返回值：  无
 ***********************************************/
void Loop_Led1(void) {
	static u32 Time_Count = 0;  //静态变量，记录时间
	static u8 flag = 0;         //静态变量，标志位
	if(millis()-Time_Count > 1000)  {
		Time_Count = millis();
		if(flag) {
			Led_On();
		} else {
			Led_Off();
		}
		flag= ~flag;
	}
}

/***********************************************
	函数名称：Loop_Usart() 
    功能介绍：串口数据接收处理
	函数参数：无
	返回值：  无
 ***********************************************/
void Loop_Usart(void) {
	if(Usart1_Get_Ok) {
		if(Usart1_Mode == 1) {					    //命令模式
			Parse_Group_CMD(Usart_Receive_Buf);
			Beep_On_times(1,100);
			Parse_CMD(Usart_Receive_Buf);			
		} else if(Usart1_Mode == 2) {			    //单个舵机调试
			Parse_Action(Usart_Receive_Buf);
		} else if(Usart1_Mode == 3) {		        //多路舵机调试
			Parse_Action(Usart_Receive_Buf);
		} else if(Usart1_Mode == 4) {		        //存储模式
			Save_Action(Usart_Receive_Buf);
		} 
		Usart1_Mode = 0;
		Usart1_Get_Ok = 0;
		Usart1_Open();
	}
	return;
}	

/*************************************************************
函数名称：Loop_PS2_Data()
功能介绍：循环读取PS2手柄数据
函数参数：无
返回值：  无  
*************************************************************/
void Loop_PS2_Data(void) {
	static u32 systick_ms_bak = 0;
	//每50ms处理1次
	if(millis() - systick_ms_bak < 50) {
		return;
	}
	systick_ms_bak = millis();
	//读写手柄数据
	psx_write_read(PS2_Buf);
	
#if 0
	//测试手柄数据，1为打开 0为关闭
	sprintf((char *)cmd_return, "0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x\r\n", 
	(int)PS2_Buf[0], (int)PS2_Buf[1], (int)PS2_Buf[2], (int)PS2_Buf[3],
	(int)PS2_Buf[4], (int)PS2_Buf[5], (int)PS2_Buf[6], (int)PS2_Buf[7], (int)PS2_Buf[8]);
	Usart1_Send_Str(cmd_return);
#endif 	
	
	return;
}	
/*************************************************************
函数名称：Loop_PS2_Buttons()
功能介绍：处理PS2手柄按钮
函数参数：无
返回值：  无  
*************************************************************/
void Loop_PS2_Buttons(void) {
	static unsigned char PS2_Button_Bak[2] = {0};

	//对比两次获取的按键值是否相同 ，相同就不处理，不相同则处理
	if((PS2_Button_Bak[0] == PS2_Buf[3])
	&& (PS2_Button_Bak[1] == PS2_Buf[4])) {				
	} else {		
		//处理buf3和buf4两个字节，这两个字节存储这手柄16个按键的状态
		Parse_PS2_Buf(PS2_Buf+3, PS2_Buf[1]);
		PS2_Button_Bak[0] = PS2_Buf[3];
		PS2_Button_Bak[1] = PS2_Buf[4];
	}
	return;
}	

/*************************************************************
函数名称：Soft_Reset()
功能介绍：处理手柄按键字符
函数参数：软件复位函数，调用后单片机自动复位
返回值：  无
*************************************************************/
void Soft_Reset(void) {
	__set_FAULTMASK(1);     
	NVIC_SystemReset();
}

/*************************************************************
函数名称：Parse_PS2_Buf()
功能介绍：处理手柄按键字符
函数参数：buf为字符数组，mode是指模式 主要是红灯和绿灯模式
返回值：  无  
*************************************************************/
void Parse_PS2_Buf(unsigned char *buf, unsigned char mode) {
	u8 i, pos = 0;
	static u16 bak=0xffff, temp, temp2;
	temp = (buf[0]<<8) + buf[1];
	
	if(bak != temp) {
		temp2 = temp;
		temp &= bak;
		for(i=0;i<16;i++) {     //16个按键一次轮询
			if((1<<i) & temp) {
			} else {
				if((1<<i) & bak) {	//press 表示按键按下了
															
                    memset(Usart_Receive_Buf, 0, sizeof(Usart_Receive_Buf));					
					if(mode == PS2_LED_RED){
						if (i == 8) {           //SELECT键按下时执行智能功能
							// 检查是否同时按下了 START → 切换到视觉自主模式
							if(PS2_START) {
								Sys_Mode = SYS_MODE_VISION;
								Auto_Mode = 0;
								Motors_Run(0,0,0,0);
								Beep_On_times(3, 100);  //响三声表示进入视觉自主模式
								while(PS2_SELECT || PS2_START) {
									psx_write_read(PS2_Buf);
								}
							} else {
								// 原有的 Auto_Mode 切换功能
								Auto_Mode += 1;
								/*
									Auto_Mode=0时，不响  ，停止
									Auto_Mode=1时，响一声，开启定距跟随功能
									Auto_Mode=2时，响两声，开启自由避障功能
									Auto_Mode=3时，响三声，开启智能循迹功能
									Auto_Mode=4时，响四声 ，开启循迹避障功能
								*/
								if(Auto_Mode > 4){
									Auto_Mode = 0;        //停止
									Motors_Run(0,0,0,0);
								}
								if(Auto_Mode == 3 || Auto_Mode == 4){  //循迹时让机械臂呈弯曲状态
									Bususart_Send_Str((u8 *)"#000P1500T1000!#001P2200T1000!#002P2300T1000!#003P1000T1000!#004P1500T1000!#005P1500T1000!\r\n");
								}
								Beep_On_times(Auto_Mode,100);
							}
						} else if (i == 9) {    //左摇杆按下时左平移
							Motors_Run(-500,500,500,-500);	
						} else if (i == 10) {   //右摇杆按下时右平移
							Motors_Run(500,-500,-500,500);
						} else if (i == 11) {   //START键按下时停止、机械臂复位
							Beep_On_times(1,100);	
							Motors_Run(0,0,0,0);	
							Auto_Mode = 0;          //停止
							Bususart_Send_Str((u8 *)"#255P1500T2000!\r\n");  //机械臂复位
						}
						memcpy((char *)Usart_Receive_Buf, (char *)pre_cmd_set_red[i], strlen(pre_cmd_set_red[i]));
					}																
					pos = Str_Contain_Str(Usart_Receive_Buf, (u8 *)"^");
					if(pos) Usart_Receive_Buf[pos-1] = '\0';
					if(Str_Contain_Str(Usart_Receive_Buf, (u8 *)"$")) {
						Usart1_Close();
						Usart1_Get_Ok = 0;
						strcpy((char *)cmd_return, (char *)Usart_Receive_Buf+11);
						strcpy((char *)Usart_Receive_Buf, (char *)cmd_return);
						Usart1_Get_Ok = 1;
						Usart1_Open();
						Usart1_Mode = 1;
					} else if(Str_Contain_Str(Usart_Receive_Buf, (u8 *)"#")) {
						Usart1_Close();
						Usart1_Get_Ok = 0;
						strcpy((char *)cmd_return, (char *)Usart_Receive_Buf+11);
						strcpy((char *)Usart_Receive_Buf,(char *) cmd_return);
						Usart1_Get_Ok = 1;
						Usart1_Open();
						Usart1_Mode = 2;
					}
					bak = 0xffff;
				} else {    //release 表示按键松开了
										
					memset(Usart_Receive_Buf, 0, sizeof(Usart_Receive_Buf));					
					if(mode == PS2_LED_RED){
						if (i == 9 || i == 10) {   //摇杆抬起时停止
							Motors_Run(0,0,0,0);	
						}
						memcpy((char *)Usart_Receive_Buf, (char *)pre_cmd_set_red[i], strlen(pre_cmd_set_red[i]));											
					}										
											
					pos = Str_Contain_Str(Usart_Receive_Buf, (u8 *)"^");
					if(pos) {
						if(Str_Contain_Str(Usart_Receive_Buf+pos, (u8 *)"$")) {
							strcpy((char *)cmd_return, (char *)Usart_Receive_Buf+pos);
							cmd_return[strlen((char *)cmd_return) - 1] = '\0';
							strcpy((char *)Usart_Receive_Buf, (char *)cmd_return);
							Parse_CMD(Usart_Receive_Buf);
							Parse_Group_CMD(Usart_Receive_Buf);
						} else if(Str_Contain_Str(Usart_Receive_Buf+pos, (u8 *)"#")) {
							strcpy((char *)cmd_return, (char *)Usart_Receive_Buf+pos);
							cmd_return[strlen((char *)cmd_return) - 1] = '\0';
							strcpy((char *)Usart_Receive_Buf, (char *)cmd_return);
							Parse_Action(Usart_Receive_Buf);

						} 

					}	
				}
			}
		}
		bak = temp2;
	}	
	return;
}

/*************************************************************
函数名称：Parse_CMD()
功能介绍：命令解析函数
函数参数：无
返回值：  无  
*************************************************************/
void Parse_CMD(u8 *cmd) {
	int pos;
/*机械臂智能控制相关指令*/
	if(pos = Str_Contain_Str(cmd, (u8 *)"$WAKE!"), pos){    	//唤醒指令
			Beep_On_times(1,100);
			Motors_Run(0,0,0,0);		
			Voice_Flag = 1;
	}else if(pos = Str_Contain_Str(cmd, (u8 *)"$QJ!"), pos){    //前进
			Beep_On_times(1,100);
			Motors_Run(600,600,600,600);		
			if (Voice_Flag == 1) {
				Delay_ms(1600);
				Motors_Run(0,0,0,0);
			}
	}else if(pos = Str_Contain_Str(cmd, (u8 *)"$HT!"), pos){	//后退
			Beep_On_times(1,100);
			Motors_Run(-600,-600,-600,-600);		
			if (Voice_Flag == 1) {
				Delay_ms(1600);
				Motors_Run(0,0,0,0);
			}
	}else if(pos = Str_Contain_Str(cmd, (u8 *)"$ZZ!"), pos){	//左转
			Beep_On_times(1,100);
			Motors_Run(-600,600,-600,600);	
			if (Voice_Flag == 1) {
				Delay_ms(500);
				Motors_Run(0,0,0,0);
			}
	}else if(pos = Str_Contain_Str(cmd, (u8 *)"$YZ!"), pos){	//右转
			Beep_On_times(1,100);
			Motors_Run(600,-600,600,-600);		
			if (Voice_Flag == 1) {
				Delay_ms(500);
				Motors_Run(0,0,0,0);
			}
	}else if(pos = Str_Contain_Str(cmd, (u8 *)"$ZPY!"), pos){	//左平移
			Beep_On_times(1,100);
			Motors_Run(-600,600,600,-600);		
			if (Voice_Flag == 1) {
				Delay_ms(2000);
				Motors_Run(0,0,0,0);
			}
	}else if(pos = Str_Contain_Str(cmd, (u8 *)"$YPY!"), pos){	//右平移
			Beep_On_times(1,100);
			Motors_Run(600,-600,-600,600);		
			if (Voice_Flag == 1) {
				Delay_ms(2000);
				Motors_Run(0,0,0,0);
			}
	}else if(pos = Str_Contain_Str(cmd, (u8 *)"$TZ!"), pos){	//停止
			Beep_On_times(1,100);
			Motors_Run(0,0,0,0);
			Auto_Mode = 0;
	}else if(pos = Str_Contain_Str(cmd, (u8 *)"$TZ!$TZ!"), pos){
			Beep_On_times(1,100);
			Motors_Run(0,0,0,0);
			Auto_Mode = 0;
	}else if(pos = Str_Contain_Str(cmd, (u8 *)"$FW!"), pos){
			Beep_On_times(1,100);
			Bususart_Send_Str((u8 *)"#255P1500T2000!\r\n");     //机械臂复位
	}else if(pos = Str_Contain_Str(cmd, (u8 *)"$QS!"), pos){    //机械臂蜷缩
			Beep_On_times(1,100);
			Bususart_Send_Str((u8 *)"#000P1500T1000!#001P2200T1000!#002P2300T1000!#003P1000T1000!#004P1500T1000!#005P1500T1000!\r\n");
	}else if(pos = Str_Contain_Str(cmd, (u8 *)"$DJGS!"), pos){  //定距跟随
			Auto_Mode = 1;
	}else if(pos = Str_Contain_Str(cmd, (u8 *)"$ZYBZ!"), pos){  //自由避障
			Auto_Mode = 2;
	}else if(pos = Str_Contain_Str(cmd, (u8 *)"$ZNXJ!"), pos){  //智能循迹
			Auto_Mode = 3;
	}else if(pos = Str_Contain_Str(cmd, (u8 *)"$XJMS!"), pos){  //智能循迹
			Auto_Mode = 3;
			Bususart_Send_Str((u8 *)"#000P1500T1000!#001P2200T1000!#002P2300T1000!#003P1000T1000!#004P1500T1000!#005P1500T1000!\r\n");  //机械臂蜷缩
	}else if(pos = Str_Contain_Str(cmd, (u8 *)"$XJBZ!"), pos){  //循迹避障
			Auto_Mode = 4;
			Bususart_Send_Str((u8 *)"#000P1500T1000!#001P2200T1000!#002P2300T1000!#003P1000T1000!#004P1500T1000!#005P1500T1000!\r\n");  //机械臂蜷缩
	}else if(pos = Str_Contain_Str(cmd, (u8 *)"$DZZ1!"), pos){  //执行动作组1
            Beep_On_times(1,100);
			Parse_Group_CMD((u8 *)"$DGT:11-16,1!");
	}else if(pos = Str_Contain_Str(cmd, (u8 *)"$DZZ2!"), pos){  //执行动作组2
            Beep_On_times(1,100);
			Parse_Group_CMD((u8 *)"$DGT:17-24,1!");
	}else if(pos = Str_Contain_Str(cmd, (u8 *)"$DZZ3!"), pos){  //执行动作组3
            Beep_On_times(1,100);
			Parse_Group_CMD((u8 *)"$DGT:3-10,1!");
	}else if(pos = Str_Contain_Str(cmd, (u8 *)"$DSQJ!"), pos){	//低速前进
			if(Voice_Flag == 1){
                Beep_On_times(1,100);
                Motors_Run(300,300,300,300);
                Delay_ms(4000);
				Motors_Run(0,0,0,0);
            }
	}else if(pos = Str_Contain_Str(cmd, (u8 *)"$ZSQJ!"), pos){	//中速前进
            if(Voice_Flag == 1){
                Beep_On_times(1,100);
                Motors_Run(600,600,600,600);
                Delay_ms(4000);
                Motors_Run(0,0,0,0);
                
            }
	}else if(pos = Str_Contain_Str(cmd, (u8 *)"$GSQJ!"), pos){	//高速前进
            if(Voice_Flag == 1){
                Beep_On_times(1,100);
                Motors_Run(900,900,900,900);
                Delay_ms(4000);
                Motors_Run(0,0,0,0);
            }
	}else if(pos = Str_Contain_Str(cmd, (u8 *)"$FX!"), pos){	//高速反向
            if(Voice_Flag == 1){
                Beep_On_times(1,100);
                Motors_Run(-900,-900,-900,-900);
                Delay_ms(4000);
                Motors_Run(0,0,0,0);
            }
	}
}

/*************************************************************
函数名称：Loop_PS2_Motors()
功能介绍：处理小车电机摇杆控制
函数参数：无
返回值：  无  
*************************************************************/
void Loop_PS2_Motors(void) {
	static int Car_Left, Car_Right, Car_Left_Bak, Car_Right_Bak;
	
	if(PS2_Buf[1] != PS2_LED_RED)
        return;
	
	if(Abs_Int(127 - PS2_Buf[8]) < 5 )
        PS2_Buf[8] = 127;
	if(Abs_Int(127 - PS2_Buf[6]) < 5 )
        PS2_Buf[6] = 127;
	
	//总线电机设置	
	Car_Left = (127 - PS2_Buf[8]) * 8;
	Car_Right = (127 - PS2_Buf[6]) * 8;
	
	if(Abs_Int(Car_Left) < 100)
        Car_Left = 0;
	if(Car_Left > 1000)
        Car_Left = 1000;
	if(Car_Left < -1000)
        Car_Left = -1000;
	
	if(Abs_Int(Car_Right) < 100)
        Car_Right = 0;
	if(Car_Right > 1000)
        Car_Right = 1000;
	if(Car_Right < -1000)
        Car_Right = -1000;
	
	if(Car_Left != Car_Left_Bak || Car_Right != Car_Right_Bak) {
		Motors_Run(Car_Left, Car_Right, Car_Left, Car_Right);
		Car_Left_Bak = Car_Left;
		Car_Right_Bak = Car_Right;
	}
}

/*************************************************************
函数名称：Loop_ColorSorting()
功能介绍：颜色识别检测
函数参数：无
返回值：  无  
*************************************************************/
void Loop_ColorSorting(void){
    Color_Sorting();
}
/*============================================================
 *  视觉自主模式 —— 非阻塞抓取状态机
 *  每步用 millis() 计时，不卡主循环，看门狗安全
 *  SELECT+START 可随时中断回到遥控模式
 *
 *  ⚠️ 所有舵机角度均为占位值，联调时根据实物逐一标定
 *============================================================*/

/* ---- 时序参数 (ms) —— 拿到车后按实测微调 ---- */
#define VA_FORWARD_MS      1200    // 前进到物块前
#define VA_ARM_UNFOLD_MS   1000    // 机械臂展开
#define VA_CLAW_OPEN_MS     400    // 夹爪张开
#define VA_ARM_DESCEND_MS   800    // 手臂下降
#define VA_CLAW_CLOSE_MS    500    // 夹爪闭合抓取
#define VA_ARM_LIFT_MS      800    // 手臂抬起
#define VA_BACKWARD_MS      800    // 后退
#define VA_TURN_MS          600    // 90°转向
#define VA_TO_ZONE_MS      1500    // 前进到配送区
#define VA_RELEASE_MS       500    // 释放物块
#define VA_ARM_RESET_MS    1000    // 手臂复位

/* ---- 电机速度 (-1000~1000) ---- */
#define VA_SPEED_FWD        600
#define VA_SPEED_BACK      -600

/* ---- 舵机指令串 (PWM 500~2500, 1500=中位) ---- */
/* 蜷缩复位（比赛开始/结束姿态） */
#define VA_CURL_CMD  "#000P1500T1000!#001P2100T1000!#002P2300T1000!#003P1000T1000!#004P1500T1000!#005P1500T1000!"
/* 展开手臂准备抓取 */
#define VA_UNFOLD_CMD "#000P1500T1000!#001P1700T1000!#002P1700T1000!#003P1300T1000!#004P1500T1000!#005P2100T1000!"
/* 手臂下降到物块高度 */
#define VA_DESCEND_CMD "#001P2000T0800!#002P2000T0800!#003P1500T0800!"
/* 夹爪闭合抓取 */
#define VA_GRAB_CMD    "#005P0900T0500!"
/* 抬起物块 */
#define VA_LIFT_CMD    "#001P1600T0800!#002P1800T0800!"
/* 底座转向配送区1（齿轮/左） */
#define VA_TURN_Z1_CMD "#000P0900T0600!"
/* 底座转向配送区2（螺母/右） */
#define VA_TURN_Z2_CMD "#000P2100T0600!"
/* 夹爪张开释放 */
#define VA_RELEASE_CMD "#005P2100T0500!"

/* ---- 抓取状态枚举 ---- */
#define VA_ST_IDLE          0
#define VA_ST_FORWARD       1
#define VA_ST_UNFOLD        2
#define VA_ST_OPEN_CLAW     3
#define VA_ST_DESCEND       4
#define VA_ST_GRAB          5
#define VA_ST_LIFT          6
#define VA_ST_BACKWARD      7
#define VA_ST_TURN          8
#define VA_ST_TO_ZONE       9
#define VA_ST_RELEASE      10
#define VA_ST_BACKWARD_2   11
#define VA_ST_TURN_BACK    12
#define VA_ST_RESET        13
#define VA_ST_DONE         14

/*************************************************************
函数名称：Loop_Vision_Auto()
功能介绍：视觉自主模式 —— 非阻塞抓取状态机
          收到ESP32指令(0x01=齿轮/0x02=螺母)后，
          自动完成：前进→抓取→后退→转向→配送→释放→复位
函数参数：无
返回值：  无
*************************************************************/
void Loop_Vision_Auto(void) {
	static u32 systick_ms_bak = 0;
	static u8  state      = VA_ST_IDLE;
	static u8  cmd_type   = 0;
	static u8  round      = 0;
	static u32 state_tmr  = 0;

	/* ---- 每50ms检测 SELECT+START 切回遥控 ---- */
	if(millis() - systick_ms_bak >= 50) {
		systick_ms_bak = millis();
		psx_write_read(PS2_Buf);

		if(PS2_SELECT && PS2_START) {
			delay_ms(100);
			psx_write_read(PS2_Buf);
			if(PS2_SELECT && PS2_START) {
				Sys_Mode = SYS_MODE_REMOTE;
				Auto_Mode = 0;
				Motors_Run(0,0,0,0);
				state    = VA_ST_IDLE;
				cmd_type = 0;
				Beep_On_times(2, 100);
				while(PS2_SELECT || PS2_START) {
					psx_write_read(PS2_Buf);
				}
				return;
			}
		}
	}

	/* ======== 抓取状态机 ======== */
	switch(state) {

		/*----- IDLE：等待 ESP32 指令 -----*/
		case VA_ST_IDLE:
			if(rx_cmd != 0) {
				cmd_type = rx_cmd;
				rx_cmd  = 0;
				if(cmd_type == 0x01)
					Beep_On_times(1, 50);   // 齿轮→响1声
				else if(cmd_type == 0x02)
					Beep_On_times(2, 50);   // 螺母→响2声
				else return;

				state     = VA_ST_FORWARD;
				state_tmr = millis();
				Motors_Run(VA_SPEED_FWD, VA_SPEED_FWD,
				           VA_SPEED_FWD, VA_SPEED_FWD);
			}
			break;

		/*----- ① 前进到物块前 -----*/
		case VA_ST_FORWARD:
			if(millis() - state_tmr >= VA_FORWARD_MS) {
				Motors_Run(0,0,0,0);
				state     = VA_ST_UNFOLD;
				state_tmr = millis();
				Bususart_Send_Str((u8 *)VA_UNFOLD_CMD);
			}
			break;

		/*----- ② 机械臂展开 -----*/
		case VA_ST_UNFOLD:
			if(millis() - state_tmr >= VA_ARM_UNFOLD_MS) {
				state     = VA_ST_OPEN_CLAW;
				state_tmr = millis();
				Bususart_Send_Str((u8 *)VA_RELEASE_CMD);
			}
			break;

		/*----- ③ 夹爪张开 -----*/
		case VA_ST_OPEN_CLAW:
			if(millis() - state_tmr >= VA_CLAW_OPEN_MS) {
				state     = VA_ST_DESCEND;
				state_tmr = millis();
				Bususart_Send_Str((u8 *)VA_DESCEND_CMD);
			}
			break;

		/*----- ④ 手臂下降到物块 -----*/
		case VA_ST_DESCEND:
			if(millis() - state_tmr >= VA_ARM_DESCEND_MS) {
				state     = VA_ST_GRAB;
				state_tmr = millis();
				Bususart_Send_Str((u8 *)VA_GRAB_CMD);
			}
			break;

		/*----- ⑤ 夹爪闭合抓取 -----*/
		case VA_ST_GRAB:
			if(millis() - state_tmr >= VA_CLAW_CLOSE_MS) {
				state     = VA_ST_LIFT;
				state_tmr = millis();
				Bususart_Send_Str((u8 *)VA_LIFT_CMD);
			}
			break;

		/*----- ⑥ 抬起物块 -----*/
		case VA_ST_LIFT:
			if(millis() - state_tmr >= VA_ARM_LIFT_MS) {
				state     = VA_ST_BACKWARD;
				state_tmr = millis();
				Motors_Run(VA_SPEED_BACK, VA_SPEED_BACK,
				           VA_SPEED_BACK, VA_SPEED_BACK);
			}
			break;

		/*----- ⑦ 后退离开 -----*/
		case VA_ST_BACKWARD:
			if(millis() - state_tmr >= VA_BACKWARD_MS) {
				Motors_Run(0,0,0,0);
				state     = VA_ST_TURN;
				state_tmr = millis();
				if(cmd_type == 0x01)
					Motors_Run(-500, 500, -500, 500);  // 齿轮→左转
				else
					Motors_Run(500, -500, 500, -500);  // 螺母→右转
			}
			break;

		/*----- ⑧ 转向配送区 -----*/
		case VA_ST_TURN:
			if(millis() - state_tmr >= VA_TURN_MS) {
				Motors_Run(0,0,0,0);
				state     = VA_ST_TO_ZONE;
				state_tmr = millis();
				Motors_Run(VA_SPEED_FWD, VA_SPEED_FWD,
				           VA_SPEED_FWD, VA_SPEED_FWD);
				if(cmd_type == 0x01)
					Bususart_Send_Str((u8 *)VA_TURN_Z1_CMD);
				else
					Bususart_Send_Str((u8 *)VA_TURN_Z2_CMD);
			}
			break;

		/*----- ⑨ 前进到配送区 -----*/
		case VA_ST_TO_ZONE:
			if(millis() - state_tmr >= VA_TO_ZONE_MS) {
				Motors_Run(0,0,0,0);
				state     = VA_ST_RELEASE;
				state_tmr = millis();
				Bususart_Send_Str((u8 *)VA_RELEASE_CMD);
			}
			break;

		/*----- ⑩ 释放物块 -----*/
		case VA_ST_RELEASE:
			if(millis() - state_tmr >= VA_RELEASE_MS) {
				state     = VA_ST_BACKWARD_2;
				state_tmr = millis();
				Motors_Run(VA_SPEED_BACK, VA_SPEED_BACK,
				           VA_SPEED_BACK, VA_SPEED_BACK);
			}
			break;

		/*----- ⑪ 后退离开配送区 -----*/
		case VA_ST_BACKWARD_2:
			if(millis() - state_tmr >= VA_BACKWARD_MS) {
				Motors_Run(0,0,0,0);
				state     = VA_ST_TURN_BACK;
				state_tmr = millis();
				if(cmd_type == 0x01)
					Motors_Run(500, -500, 500, -500);   // 右转回
				else
					Motors_Run(-500, 500, -500, 500);   // 左转回
			}
			break;

		/*----- ⑫ 转回中继区方向 -----*/
		case VA_ST_TURN_BACK:
			if(millis() - state_tmr >= VA_TURN_MS) {
				Motors_Run(0,0,0,0);
				state     = VA_ST_RESET;
				state_tmr = millis();
				Bususart_Send_Str((u8 *)VA_CURL_CMD);
			}
			break;

		/*----- ⑬ 手臂复位蜷缩 -----*/
		case VA_ST_RESET:
			if(millis() - state_tmr >= VA_ARM_RESET_MS) {
				state     = VA_ST_DONE;
				state_tmr = millis();
			}
			break;

		/*----- ⑭ 本轮完成，检查是否满8轮 -----*/
		case VA_ST_DONE:
			round++;
			if(round >= 8) {
				Beep_On_times(3, 200);   // 全部完成→响3声
				state = VA_ST_IDLE;
				round = 0;
			} else {
				state = VA_ST_IDLE;      // 回IDLE等下一轮
			}
			cmd_type = 0;
			break;

		default:
			state = VA_ST_IDLE;
			break;
	}
}

/*************************************************************
函数名称：Motors_Run()
功能介绍：发送串口指令控制电机转动
函数参数：左前轮速度，右前轮速度，左后轮速度，右后轮速度，范围：-1000~1000， 负值反转，正值正转
返回值：  无  
*************************************************************/
void Motors_Run(int Left_Front_Speed, int Right_Front_Speed, int Left_Back_Speed, int Right_Back_Speed) {	
	sprintf((char *)cmd_return, "{#006P%04dT0000!#007P%04dT0000!#008P%04dT0000!#009P%04dT0000!}", (int)(1500+Left_Front_Speed), (int)(1500-Right_Front_Speed), (int)(1500+Left_Back_Speed), (int)(1500-Right_Back_Speed));
	Bususart_Send_Str(cmd_return);
	return;
}
