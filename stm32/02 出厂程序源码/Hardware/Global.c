#include "stm32f10x_conf.h"
#include "Global.h"



duoji_t Servos_Doing[Servos_NUM];
u8 cmd_return[CMD_RETURN_SIZE];
u8 Usart_Receive_Buf[UART_BUF_SIZE], Usart1_Get_Ok, Usart1_Mode;
eeprom_info_t eeprom_info;
u8 Group_Do_Complete = 1;


void Global_Init(void) { //初始化舵机参数
	u8 i;
	//舵机控制初始化
	for(i=0;i<Servos_NUM;i++) {
		Servos_Doing[i].aim = 1500;  //目标1500中位
		Servos_Doing[i].cur = 1500;  //当前值1500
		Servos_Doing[i].inc = 0;     //增量0
	}
	return;
}

/***********************************************
	函数名称：Str_Contain_Str() 
	功能介绍：判断一个字符串是否包含另外一个字符串
	函数参数：字符串
	返回值：  str_temp-str 或 0
 ***********************************************/
uint16_t Str_Contain_Str(unsigned char *str, unsigned char *str2) {
	unsigned char *str_temp, *str_temp2;
	str_temp = str;
	str_temp2 = str2;
	while(*str_temp) {
		if(*str_temp == *str_temp2) {
			while(*str_temp2) {
				if(*str_temp++ != *str_temp2++) {
					str_temp = str_temp - (str_temp2-str2) + 1;
					str_temp2 = str2;
					break;
				}	
			}
			if(!*str_temp2) {
				return (str_temp-str);
			}
			
		} else {
			str_temp++;
		}
	}
	return 0;
}

void selection_sort(int *a, int len) {
    int i,j,mi,t;
    for(i=0;i<len-1;i++) {
        mi = i;
        for(j=i+1;j<len;j++) {
            if(a[mi] > a[j]) {
                mi = j;    
            }    
        }    
		
        if(mi != i) {
            t = a[mi];
            a[mi] = a[i];
            a[i] = t;    
        }
    }
}

//int型 取绝对值函数
int Abs_Int(int int1) {
	if(int1 > 0)return int1;
	return (-int1);
}

