#ifndef __MAIN_H__
#define __MAIN_H__

#include "stm32f10x_conf.h"
#include "Sensor.h"
#include "usart.h"

/*
	宏定义数据
*/
#define VERSION	        20220728	//版本定义
#define ACTION_USE_ROM	0			   //1:动作组使用内部数组动作组	0:动作组使用上位机下载动作组
#define CYCLE 			1000		//PWM模块周期
#define PS2_LED_RED     0x73		//PS2手柄红灯模式
#define PS2_LED_GRN  	0x41		//PS2手柄绿灯模式
#define PSX_BUTTON_NUM 	16			//手柄按键数目
#define PS2_MAX_LEN 	64			//手柄命令最大字节数
#define FLAG_VERIFY 	0x25		//校验标志
//#define ACTION_SIZE 	0x80		//一个动作的存储大小

#define W25Q64_INFO_ADDR_SAVE_STR			(((8<<10)-2)<<10)  //(8*1024-1)*1024		//eeprom_info结构体存储的位置

/*******全局变量外部声明*******/
extern u8 Auto_Mode;
extern u8 Sys_Mode;
extern u8 rx_cmd;
extern int Tracking_Speed;

/*
	初始化函数声明
*/

void setup_rcc(void);       //初始化时钟
void setup_global(void);    //初始化全局变量		
void setup_gpio(void);      //初始化IO口			
void setup_nled(void);      //初始化工作指示灯
void setup_beep(void);	    //初始化蜂鸣器		
void setup_djio(void);	    //初始化舵机IO口
void W25Q64_Init(void);	    //初始化存储器W25Q64
void setup_ps2(void);       //初始化PS2手柄
void setup_uart1(void);	    //初始化串口1
void setup_uart2(void);	    //初始化串口2
void setup_uart3(void);	    //初始化串口3
void setup_systick(void);   //初始化滴答时钟，1S增加一次systick_ms的值
void Starting_up(void);	    //初始化启动信号
void setup_interrupt(void); //初始化总中断
/*
	主循环函数声明
*/
void Loop_Led1(void);	        //循环执行工作指示灯，500ms跳动一次	
void Loop_Usart(void);	        //串口数据接收处理
void Loop_Actions(void);	    //动作组批量执行
void loop_bt_once(void);	    //蓝牙修改波特率和名称	
void Loop_PS2_Data(void);	    //循环读取PS2手柄数据
void Loop_PS2_Buttons(void);	//处理手柄上的按钮
void Loop_PS2_Motors(void);     //处理小车电机摇杆控制
void loop_vol_warning(void);    //处理低压报警
void loop_inc(void);			//循环处理增量
void Loop_Sonser(void);         //循环传感器检测
void Loop_ColorSorting(void);   //循环颜色识别传感器检测
void Loop_Vision_Auto(void);    //视觉自主模式处理（新增）

void Soft_Reset(void);		    //软件复位

void Parse_PS2_Buf(u8 *buf, u8 mode);     //手柄按键解析子函数
void Parse_CMD(u8 *cmd);			      //命令解析
void Motors_Run(int Left_Front_Speed, int Right_Front_Speed, int Left_Back_Speed, int Right_Back_Speed);  
//int kinematics_move(float x, float y, float z, int time);//逆运动学转动

#endif
