#ifndef __ACTION_H__
#define __ACTION_H__

#include "stm32f10x_conf.h"
#include "Usart.h"

#define CAR_PWM				0
#define ADC_VOL				7
#define ACTION_SIZE			256
#define ACTION_FLAG_VERIFY 	0x38
#define ACTION_W25Q64_INFO_ADDR_SAVE_STR	(((8<<10)-4)<<10)//(8*1024-4)*1024		//eeprom_info结构体存储的位置

/*初始化函数声明*/			
void Servos_Bias_Init(void);    //初始化舵机偏差
void Starting_Group(void);      //执行开机动作组
void Loop_Actions(void);	    //动作组批量执行	

/*子循环函数声明*/
void Parse_Group_CMD(u8 *cmd);              //解析 $ 开头 ! 结尾的指令
void Save_Action(u8 *str);                  //动作组保存函数
int Action_Get_Index(u8 *str);              //获取动作序号
void Print_Group(int start, int end);       //打印存储器中的动作组
void int_exchange(int *int1, int *int2);    //数据交换函数
void Group_Run_Once(int Group_Num);         //执行存储器中的Group_Num号动作组
void Parse_Action(u8 *Usart_Receive_Buf);   //解析以#开头！结尾的指令
void replace_char(u8*str, u8 ch1, u8 ch2);  //字符串中的字符替代函数
void rewrite_eeprom(void);                  //把eeprom_info写入到ACTION_W25Q64_INFO_ADDR_SAVE_STR位置
void Set_Servos(int index, int pwm, int time);	//设置舵机角度

#endif
