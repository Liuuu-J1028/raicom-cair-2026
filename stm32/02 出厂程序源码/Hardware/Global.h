#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include "stm32f10x_conf.h"


extern u8 Auto_Mode;
extern u8 Sys_Mode;
extern u8 rx_cmd;
extern u8 Group_Do_Complete;


#define Servos_NUM 8

typedef struct {
	uint8_t 	valid;//有效 TODO	
	uint16_t 	aim;	//执行目标
	uint16_t 	time;	//执行时间		
	float 		cur;	//当前值
	float 		inc;	//增量	
}duoji_t;


#define PRE_CMD_SIZE 128
typedef struct {
	u32 version;
	u32 dj_record_num;
	u8  pre_cmd[PRE_CMD_SIZE + 1];
	int dj_bias_pwm[Servos_NUM+1];
	u8 color_base_flag;
	int color_red_base;
	int color_grn_base;
	int color_blu_base;
}eeprom_info_t;



extern duoji_t Servos_Doing[Servos_NUM];
extern uint8_t Servos_Index1;
extern uint8_t Tim2_Stop;

#define CMD_RETURN_SIZE 1024
extern u8 cmd_return[CMD_RETURN_SIZE];
#define UART_BUF_SIZE 1024
extern u8 Usart_Receive_Buf[UART_BUF_SIZE], Usart1_Get_Ok, Usart1_Mode;
extern eeprom_info_t eeprom_info;

void Global_Init(void);
uint16_t Str_Contain_Str(unsigned char *str, unsigned char *str2);
int Abs_Int(int int1);
void selection_sort(int *a, int len);

#endif


