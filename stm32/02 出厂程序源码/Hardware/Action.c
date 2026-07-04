/*
	动作组控制相关函数
	
	指令表：
	$DST!              所有舵机停在当前位置，包括总线舵机和pwm舵机
	$DST:x!            x号舵机停在当前位置，包括总线舵机和pwm舵机
	$RST!              软件复位
	$PGP:%d-%d!        将存储器中的动作组从USB（uart1）打印出来     
	$DGS:x!            执行GXXXX一组动作组
	$DGT:%d-%d,%d!     执行x-x动作组x次
	$DJR!              所有舵机复位
	$GETA!
		
	波特率 22.1184	
*/
#include "stm32f10x.h"                  // Device header

#include "stm32f10x_iwdg.h"
#include "Rcc.h"		//配置时钟文件
#include "Gpio.h"		//配置IO口文件
#include "Global.h"	    //存放全局变量
#include "Delay.h"	    //存放延时函数
#include "Type.h"		//存放类型定义
#include "Usart.h"	    //存放串口功能文件
#include "Timer.h"	    //存放定时器功能文件
#include "PS2.h"		//存放索尼手柄
#include "W25q64.h"	    //存储芯片的操作
#include <stdio.h>		//标准库文件
#include <string.h>		//标准库文件
#include <math.h>	  	//标准库文件
#include "Kinematics.h"	//逆运动学算法
#include "Action.h"     //动作组相关文件

int bias;                 
int Group_Start_Index;		    //动作组执行 起始序号
int Group_Do_Times;			    //动作组执行 执行次数
int Group_Num_start;	        //动作组执行 起始序号
int Group_Num_end;		        //动作组执行 终止序号
int Group_Num_times;			//动作组执行 起始变量

u32 Action_time = 0;            //动作组执行时间间隔

/***********************************************
	函数名称：Servos_Bias_Init() 
	功能介绍：初始化舵机，将偏差带入初始值
	函数参数：无
	返回值：	无
 ***********************************************/
void Servos_Bias_Init(void) {
	u8 i;
	for(i=0;i<Servos_NUM;i++) {
		Servos_Doing[i].aim = 1500+eeprom_info.dj_bias_pwm[i];
		Servos_Doing[i].cur = Servos_Doing[i].aim;
		Servos_Doing[i].inc = 0;		
	}
}
/***********************************************
	函数名称：Loop_Actions() 
    功能介绍：执行开机动作
	函数参数：无
	返回值：  无
 ***********************************************/
void Starting_Group(void) {
	u8 i;
	Bususart_Send_Str((u8 *)"#000P1500T1000!#001P2100T1000!#002P2300T1000!#003P1000T1000!#004P1500T1000!#005P1500T1000!\r\n");  //机械臂蜷缩
	//执行预存命令 {G0000#000P1500T1000!#000P1500T1000!}
	if(eeprom_info.pre_cmd[PRE_CMD_SIZE] == ACTION_FLAG_VERIFY) {
		strcpy((char *)Usart_Receive_Buf, (char *)eeprom_info.pre_cmd);
		if(eeprom_info.pre_cmd[0] == '$') {
			Parse_Group_CMD(eeprom_info.pre_cmd);
		} else {
			for(i=16;i<strlen((char *)Usart_Receive_Buf);i+=15) {
				Usart_Receive_Buf[i] = '0';
				Usart_Receive_Buf[i+1] = '0';
				Usart_Receive_Buf[i+2] = '0';
				Usart_Receive_Buf[i+3] = '0';
			}
			Parse_Action(Usart_Receive_Buf);
		}
	}
}

/***********************************************
	函数名称：Loop_Actions() 
	功能介绍：执行动作组指令
	函数参数：无
	返回值：	无
 ***********************************************/
void Loop_Actions(void) {
	static u32 systick_ms_bak = 0;
	if(Group_Do_Complete == 0) {
		if(millis() - systick_ms_bak > Action_time) {
			systick_ms_bak =  millis();
			if(Group_Num_times != 0 && Group_Do_Times == 0) {
			  Group_Do_Complete = 1;
			  Usart1_Send_Str((u8 *)"@GroupDone!");
			  return;
			}
			//调用Group_Start_Index个动作
			Group_Run_Once(Group_Start_Index);
			
			if(Group_Num_start<Group_Num_end) {
				if(Group_Start_Index == Group_Num_end) {
					Group_Start_Index = Group_Num_start;
					if(Group_Num_times != 0) {
						Group_Do_Times--;
					}
					return;
				}
				Group_Start_Index++;
			} else {
				if(Group_Start_Index == Group_Num_end) {
					Group_Start_Index = Group_Num_start;
					if(Group_Num_times != 0) {
						Group_Do_Times--;
					}
					return;
				}
				Group_Start_Index--;
			}
		}
	} else {
      Action_time = 10;
	}
}

/***********************************************
	函数名称：Parse_Group_CMD() 
	功能介绍：解析 $ 开头 ! 结尾的指令，处理动作组相关指令
	函数参数：字符串指令表中的指令
	返回值：  无
 ***********************************************/
void Parse_Group_CMD(u8 *cmd) {
	int pos, i, index, int1, int2;

	if(pos = Str_Contain_Str(cmd, (u8 *)"$DRS!"), pos)
        {
            Usart1_Send_Str((u8 *)"module:");
            Usart1_Send_Str((u8 *)"\r\n");
            Usart1_Send_Str((u8 *)"bias:");
            for(i=0;i<Servos_NUM;i++)
                {
                    sprintf((char*)cmd_return, "%d  ", eeprom_info.dj_bias_pwm[i]);
                    Usart1_Send_Str(cmd_return);
                }
            Usart1_Send_Str((u8 *)"\r\n");
            Usart1_Send_Str((u8 *)"preCmd:");
            Usart1_Send_Str( eeprom_info.pre_cmd);
		
        }else if(pos = Str_Contain_Str(cmd, (u8 *)"$DST!"), pos)
            {
                Group_Do_Complete  = 1;
                for(i=0;i<Servos_NUM;i++) {
                    Servos_Doing[i].inc = 0;	
                    Servos_Doing[i].aim = Servos_Doing[i].cur;
                }
                Bususart_Send_Str((u8 *)"#255PDST!");//总线停止
	} else if(pos = Str_Contain_Str(cmd, (u8 *)"$DST:"), pos) {
		if(sscanf((char *)cmd, "$DST:%d!", &index)) {
			Servos_Doing[index].inc = 0;	
			Servos_Doing[index].aim = Servos_Doing[index].cur;
			sprintf((char *)cmd_return, "#%03dPDST!\r\n", (int)index);
			Bususart_Send_Str(cmd_return);
			memset(cmd_return, 0, sizeof(cmd_return));
		}
	} else if(pos = Str_Contain_Str(cmd, (u8 *)"$RST!"), pos) {
			Soft_Reset();
	} else if(pos = Str_Contain_Str(cmd, (u8 *)"$PTG:"), pos) {		
		if(sscanf((char *)cmd, "$PTG:%d-%d!", &int1, &int2)) {
			Print_Group(int1, int2);
		}
	} else if(pos = Str_Contain_Str(cmd, (u8 *)"$DGS:"), pos) {		
		if(sscanf((char *)cmd, "$DGS:%d!", &int1)) {
			Group_Do_Complete = 1;
			Group_Run_Once(int1);
		}
	} else if(pos = Str_Contain_Str(cmd, (u8 *)"$DGT:"), pos) {		
		if(sscanf((char *)cmd, "$DGT:%d-%d,%d!", &Group_Num_start, &Group_Num_end, &Group_Num_times)) {		
			if(Group_Num_start != Group_Num_end) {
				Group_Start_Index = Group_Num_start;
				Group_Do_Times = Group_Num_times;
				Group_Do_Complete = 0;
			} else {
				Group_Run_Once(Group_Num_start);
			}
		}
	} else if(pos = Str_Contain_Str(cmd, (u8 *)"$DJR!"), pos) {	
		Bususart_Send_Str((u8 *)"#255P1500T2000!\r\n");
		for(i=0;i<Servos_NUM;i++) {
			Servos_Doing[i].aim = 1500+eeprom_info.dj_bias_pwm[i];
			Servos_Doing[i].time = 2000;
			Servos_Doing[i].inc = (Servos_Doing[i].aim -  Servos_Doing[i].cur) / (Servos_Doing[i].time/20.000);
		}		
	} else if(pos = Str_Contain_Str(cmd, (u8 *)"$GETA!"), pos) {		
			Usart1_Send_Str((u8 *)"AAA");
	} 
}


/***********************************************
	函数名称：Save_Action() 
    功能介绍：动作组保存函数，设置开机动作组
	函数参数：只有用<>包含的字符串才能在此函数中进行解析
	返回值：	无
 ***********************************************/
void Save_Action(u8 *str) {
	int action_index = 0;
	//预存命令处理
	spiFlahsOn(1);
	if(str[1] == '$' && str[2] == '!') {
		eeprom_info.pre_cmd[PRE_CMD_SIZE] = 0;
		rewrite_eeprom();   //去掉开机动作组
		Usart1_Send_Str((u8 *)"@CLEAR PRE_CMD OK!");
		return;
	}else if(str[1] == '$') {
		//设置开机动作组
		if(sscanf((char *)str, "<$DGT:%d-%d,%d!>", &Group_Num_start, &Group_Num_end, &Group_Num_times)) {
			if(Group_Num_start == Group_Num_end) {
				w25x_read(eeprom_info.pre_cmd, Group_Num_start*ACTION_SIZE, ACTION_SIZE);	
			} else {
				memset(eeprom_info.pre_cmd, 0, sizeof(eeprom_info.pre_cmd));
				strcpy((char *)eeprom_info.pre_cmd, (char *)str+1);
				eeprom_info.pre_cmd[strlen((char *)str) - 2] = '\0';
			}
			eeprom_info.pre_cmd[PRE_CMD_SIZE] = ACTION_FLAG_VERIFY;
			rewrite_eeprom();
			//Usart1_Send_Str(eeprom_info.pre_cmd);
			Usart1_Send_Str((u8 *)"@SET PRE_CMD OK!");
		}
		return;
	}
	
	action_index = Action_Get_Index(str);
	if((action_index == -1) || str[6] != '#'){
		Usart1_Send_Str((u8*)"E");
		return;
	}
	
	replace_char(str, '<', '{');
	replace_char(str, '>', '}');
	
	if(action_index*ACTION_SIZE % W25Q64_SECTOR_SIZE == 0)w25x_erase_sector(action_index*ACTION_SIZE/W25Q64_SECTOR_SIZE);

	w25x_write(str, action_index*ACTION_SIZE, strlen((const char *)str) + 1);
	Usart1_Send_Str((u8*)"A");
	spiFlahsOn(0);
	return;	
}

/***********************************************
	函数名称：Action_Get_Index() 
	功能介绍：获取动作组序号
	函数参数：无
	返回值：  无
 ***********************************************/
int Action_Get_Index(u8 *str) {
	int index = 0;

	while(*str) {
		if(*str == 'G') {
			str++;
			while((*str != '#') && (*str != '$')) {
				index = index*10 + *str-'0';
				str++;	
			}
			return index;
		} else {
			str++;
		}
	}
	return -1;
}

/***********************************************
	函数名称：Print_Group() 
	功能介绍：打印存储器中的动作组
	函数参数：起始位置，终止位置
	返回值：	无
 ***********************************************/
void Print_Group(int start, int end) {
	spiFlahsOn(1);
	
	if(start > end) {
		int_exchange(&start, &end);
	}
	
	for(;start<=end;start++) {
		memset(Usart_Receive_Buf, 0, sizeof(Usart_Receive_Buf));
		w25x_read(Usart_Receive_Buf, start*ACTION_SIZE, ACTION_SIZE);
		Usart1_Send_Str(Usart_Receive_Buf);
		Usart1_Send_Str((u8 *)"\r\n");
		
		Usart3_Send_Str(Usart_Receive_Buf);
		Usart3_Send_Str((u8 *)"\r\n");
	}
	
	spiFlahsOn(0);
}

/***********************************************
	函数名称：int_exchange() 
	功能介绍：数据交换
	函数参数：无
	返回值：  无
 ***********************************************/
void int_exchange(int *int1, int *int2) {
	int int_temp;
	int_temp = *int1;
	*int1 = *int2;
	*int2 = int_temp;
}

/***********************************************
	函数名称：GetMaxTime() 
	功能介绍：获取最大时间
	函数参数：无
	返回值：	无
 ***********************************************/
int GetMaxTime(u8 *str) {
   int i = 0, max_time = 0, tmp_time = 0;
   while(str[i]) {
      if(str[i] == 'T') {
          tmp_time = (str[i+1]-'0')*1000 + (str[i+2]-'0')*100 + (str[i+3]-'0')*10 + (str[i+4]-'0');
          if(tmp_time>max_time)max_time = tmp_time;
          i = i+4;
          continue;
      }
      i++;
   }
   return max_time;
}

/***********************************************
	函数名称：Group_Run_Once() 
	功能介绍：从存储芯片中读取第Group_Num个动作组，执行动作组
	函数参数：动作组序号
	返回值：	无
 ***********************************************/
void Group_Run_Once(int Group_Num) {
	spiFlahsOn(1);
	//将Usart_Receive_Buf清零
	memset(Usart_Receive_Buf, 0, sizeof(Usart_Receive_Buf));
	//从存储芯片中读取第Group_Num个动作组
	w25x_read(Usart_Receive_Buf, Group_Num*ACTION_SIZE, ACTION_SIZE-1);	
	//获取最大的组时间
	Action_time = GetMaxTime(Usart_Receive_Buf);
	
	//把读取出来的动作组传递到Parse_Action执行
	Parse_Action(Usart_Receive_Buf);
	spiFlahsOn(0);
}

/***********************************************
	函数名称：Parse_Action() 
	功能介绍：解析以#开头！结尾的指令
	函数参数：无
	返回值：  无
 ***********************************************/
void Parse_Action(u8 *Usart_Receive_Buf) {
	u16 index, time, i = 0, j, step = 0;
	float pwm;
	float Aim_Temp;
	Bususart_Send_Str(Usart_Receive_Buf);
	//解析#000PSCK+001!指令，调节pwm舵机偏差
	if(Usart_Receive_Buf[0] == '#' && Usart_Receive_Buf[4] == 'P' && Usart_Receive_Buf[5] == 'S' && Usart_Receive_Buf[6] == 'C' && Usart_Receive_Buf[7] == 'K' && Usart_Receive_Buf[12] == '!') {
		index = (Usart_Receive_Buf[1] - '0')*100 + (Usart_Receive_Buf[2] - '0')*10 + (Usart_Receive_Buf[3] - '0');
		bias = (Usart_Receive_Buf[9] - '0')*100 + (Usart_Receive_Buf[10] - '0')*10 + (Usart_Receive_Buf[11] - '0');
		if((bias >= -500) && (bias <= 500) && (index < Servos_NUM)) {
			if(Usart_Receive_Buf[8] == '+') {
			} else if(Usart_Receive_Buf[8] == '-') {
				bias = -bias;
			}
				
			Aim_Temp = Servos_Doing[index].cur - (eeprom_info.dj_bias_pwm[index] - bias);
			eeprom_info.dj_bias_pwm[index] = bias; //将偏差记录到eeprom中
			//bias_bak = bias;
			if(Aim_Temp > 2497){
				Aim_Temp = 2497;
			} else if(Aim_Temp < 500) {
				Aim_Temp = 500;
			}
			//转动舵机，可看出当前偏差
			Servos_Doing[index].aim = Aim_Temp;
			Servos_Doing[index].inc = (Servos_Doing[index].aim - Servos_Doing[index].cur)/5;
			rewrite_eeprom();			
		}
		return;
            //解析#000PDST!指令，使PWM舵机停止在当前位置
	} else if(Usart_Receive_Buf[0] == '#' && Usart_Receive_Buf[4] == 'P' && Usart_Receive_Buf[5] == 'D' && Usart_Receive_Buf[6] == 'S' && Usart_Receive_Buf[7] == 'T' && Usart_Receive_Buf[8] == '!') {
		index = (Usart_Receive_Buf[1] - '0')*100 + (Usart_Receive_Buf[2] - '0')*10 + (Usart_Receive_Buf[3] - '0');		
		if(index < Servos_NUM) {
			Servos_Doing[index].inc = 0;	
			Servos_Doing[index].aim = Servos_Doing[index].cur;
		}
		return;
	}
	
	step = 1;
	while(Usart_Receive_Buf[i]) {
		if(Usart_Receive_Buf[i] == '#' && step == 1) {
			j = i;
			index = 0;i++;
			while(Usart_Receive_Buf[i] && Usart_Receive_Buf[i] != 'P') {
				index = index*10 + Usart_Receive_Buf[i]-'0';i++;
			}
			if(i-j-1 != 3) {
				step = 1;
			} else {
				step = 2;
			}
		} else if(Usart_Receive_Buf[i] == 'P' && step == 2) {
			j = i;
			pwm = 0;i++;
			while(Usart_Receive_Buf[i] && Usart_Receive_Buf[i] != 'T') {
				pwm = pwm*10 + Usart_Receive_Buf[i]-'0';i++;
			}
			if(i-j-1 != 4) {
				step = 1;
			} else {
				step = 3;
			}
		} else if(Usart_Receive_Buf[i] == 'T' && step == 3) {
			j = i;
			time = 0;i++;
			while(Usart_Receive_Buf[i] && Usart_Receive_Buf[i] != '!') {
				time = time*10 + Usart_Receive_Buf[i]-'0';i++;
			}
			
			//同步的时候防止数据太快不稳定
			if(time<500) {
				time = time+300;
			}
			
			step = 1;
			if(i-j-1 != 4) {
			} else {
				if(index < Servos_NUM && (pwm<=2500)&& (pwm>=500) && (time<=10000)) {
					pwm += eeprom_info.dj_bias_pwm[index];
					
					if(pwm>2497)pwm=2497;
					if(pwm<500)pwm=500;
					
					if(time < 20) {
						Servos_Doing[index].aim = pwm;
						Servos_Doing[index].cur = pwm;
						Servos_Doing[index].inc = 0;
					} else {
						Servos_Doing[index].aim = pwm;
						Servos_Doing[index].time = time;
						Servos_Doing[index].inc = (Servos_Doing[index].aim -  Servos_Doing[index].cur) / (Servos_Doing[index].time/20.000);
					}
				} 
				
				if(index == 255) {
					for(index=0;index<Servos_NUM;index++) {
						pwm =1500 + eeprom_info.dj_bias_pwm[index];
						Servos_Doing[index].aim = pwm;
						Servos_Doing[index].time = time;
						Servos_Doing[index].inc = (Servos_Doing[index].aim -  Servos_Doing[index].cur) / (Servos_Doing[index].time/20.000);
					}
				}
			}
		} else {
			i++;
		}
	}	
}

/***********************************************
	函数名称：replace_char() 
	功能介绍：字符串中的字符替代函数,把str字符串中所有的ch1换成ch2
	函数参数：字符串
	返回值：	无
 ***********************************************/
void replace_char(u8*str, u8 ch1, u8 ch2) {
	while(*str) {
		if(*str == ch1) {
			*str = ch2;
		} 
		str++;
	}
	return;
}

/***********************************************
	函数名称：rewrite_eeprom() 
	功能介绍：把eeprom_info写入到ACTION_W25Q64_INFO_ADDR_SAVE_STR位置
	函数参数：无
	返回值：  无
 ***********************************************/
void rewrite_eeprom(void) {
	spiFlahsOn(1);
	w25x_erase_sector(ACTION_W25Q64_INFO_ADDR_SAVE_STR/4096);
	w25x_writeS((u8 *)(&eeprom_info), ACTION_W25Q64_INFO_ADDR_SAVE_STR, sizeof(eeprom_info));
	spiFlahsOn(0);
}

/***********************************************
	函数名称：Set_Servos() 
	功能介绍：舵机控制
	函数参数：id，pwm值，转动时间
	返回值：	无
 ***********************************************/
void Set_Servos(int index, int pwm, int time) {
	Servos_Doing[index].aim = pwm;
	Servos_Doing[index].time = time;
	Servos_Doing[index].inc = (Servos_Doing[index].aim -  Servos_Doing[index].cur) / (Servos_Doing[index].time/20.000);
	sprintf((char *)cmd_return, "#%03dP%04dT%04d!\r\n", index, pwm, time);
	Bususart_Send_Str(cmd_return);	
}
