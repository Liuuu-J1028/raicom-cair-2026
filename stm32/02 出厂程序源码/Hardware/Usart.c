#include "Usart.h"
#include "Delay.h"

void Usart_Init(void) {
	Usart1_Init(115200);
	Usart1_Open();
	
	Usart2_Init(115200);
	Usart2_Open();
	
	Usart3_Init(115200);
	Usart3_Open();
	
	Interrupt_Open();
	return;
}

void Usart1_Init(u32 rate) {  
    GPIO_InitTypeDef GPIO_InitStructure;  
    USART_InitTypeDef USART_InitStructure; 
	USART_ClockInitTypeDef USART_ClockInitStructure; 	
    NVIC_InitTypeDef NVIC_InitStructure;  
  
	
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);  
    USART_DeInit(USART1);  
    /* Configure USART Tx as alternate function push-pull */  
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;  
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;  
    GPIO_Init(GPIOA, &GPIO_InitStructure);  
      
    /* Configure USART Rx as input floating */  
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;  
    GPIO_Init(GPIOA, &GPIO_InitStructure);  
  
    USART_InitStructure.USART_BaudRate = rate;  
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;  
    USART_InitStructure.USART_StopBits = USART_StopBits_1;  
    USART_InitStructure.USART_Parity = USART_Parity_No;  
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;  
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;  
    
	USART_ClockInitStructure.USART_Clock = USART_Clock_Disable;  
    USART_ClockInitStructure.USART_CPOL = USART_CPOL_Low;  
    USART_ClockInitStructure.USART_CPHA = USART_CPHA_2Edge;  
    USART_ClockInitStructure.USART_LastBit = USART_LastBit_Disable;  
    USART_ClockInit(USART1, &USART_ClockInitStructure);  
	USART_Init(USART1, &USART_InitStructure );   
  
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;  
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;  
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;  
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;  
    NVIC_Init(&NVIC_InitStructure); 
	
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    
	USART_Cmd(USART1, ENABLE);  
}  
  
void Usart2_Init(u32 rate) {  
	GPIO_InitTypeDef GPIO_InitStructure;  
    USART_InitTypeDef USART_InitStructure;   
	NVIC_InitTypeDef NVIC_InitStructure; 
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);  
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);  
    USART_DeInit(USART2);  
    /* Configure USART Tx as alternate function push-pull */  
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;  
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;  
    GPIO_Init(GPIOA, &GPIO_InitStructure);  
      
    /* Configure USART Rx as input floating */  
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;  
    GPIO_Init(GPIOA, &GPIO_InitStructure);  
	
    USART_InitStructure.USART_BaudRate = rate;  
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;  
    USART_InitStructure.USART_StopBits = USART_StopBits_1;  
    USART_InitStructure.USART_Parity = USART_Parity_No;  
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;  
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;  
	USART_Init(USART2, &USART_InitStructure );   
	
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);   
	
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
	
	USART_Cmd(USART2, ENABLE); 
} 

void Usart3_Init(u32 rate) {  
	GPIO_InitTypeDef GPIO_InitStructure;  
    USART_InitTypeDef USART_InitStructure;   
	NVIC_InitTypeDef NVIC_InitStructure; 
	
	
	/* config USART3 clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
	
	/* USART3 GPIO config */
	/* Configure USART3 Tx (PB.10) as alternate function push-pull */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);    
	
	/* Configure USART3 Rx (PB.11) as input floating */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	/* USART3 mode config */
	USART_InitStructure.USART_BaudRate = rate;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No ;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART3, &USART_InitStructure);
		
	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);   
	
	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);

	USART_Cmd(USART3, ENABLE); 
} 

//从串口1发送一个字节
void Usart1_Send_Byte(u8 Data) {
	USART_SendData(USART1, Data);
	return;
}

void Usart1_Send_nByte(u8 *Data, u16 size) {
	u16 i = 0;
	for(i=0; i<size; i++) {
		USART_SendData(USART1, Data[i]);
		while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET); 
	}
	return;
}
void Usart1_Send_Str(u8 *Data) {
	while(*Data) {
		USART_SendData(USART1, *Data++);
		while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET); 
	}
	return;
}

//从串口2发送一个字节
void Usart2_Send_Byte(u8 Data) {
	USART_SendData(USART2, Data);
	return;
}

void Usart2_Send_nByte(u8 *Data, u16 size) {
	u16 i = 0;
	for(i=0; i<size; i++) {
		USART_SendData(USART2, Data[i]);
		while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET); 
	}
	return;
}
void Usart2_Send_Str(u8 *Data) {
	while(*Data) {
		USART_SendData(USART2, *Data++);
		while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET); 
	}
	return;
}

//从串口3发送一个字节
void Usart3_Send_Byte(u8 Data) {
	USART_SendData(USART3, Data);
	return;
}

void Usart3_Send_nByte(u8 *Data, u16 size) {
	u16 i = 0;
	for(i=0; i<size; i++) {
		USART_SendData(USART3, Data[i]);
		while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET); 
	}
	return;
}
void Usart3_Send_Str(u8 *Data) {
	while(*Data) {
		USART_SendData(USART3, *Data++);
		while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET); 
	}
	return;
}


/**========================中断处理函数=============================**/

/***********************************************
	函数名称：USART1_IRQHandler() 
	功能介绍：串口1收发中断处理函数
	函数参数：无
	返回值：	无
 ***********************************************/
int USART1_IRQHandler(void) {
	u8 Sbuf_Bak;
	static u16 Buf_Index = 0;

	if(USART_GetFlagStatus(USART1,USART_IT_RXNE)==SET) {
		USART_ClearITPendingBit(USART1, USART_IT_RXNE);		
		Sbuf_Bak = USART_ReceiveData(USART1); 
		if(Usart1_Get_Ok)return 0;
		if(Sbuf_Bak == '<') {
			Usart1_Mode = 4;
			Buf_Index = 0;
		}else if(Usart1_Mode == 0) {
			if(Sbuf_Bak == '$') {			//命令模式 $XXX!
				Usart1_Mode = 1;
			} else if(Sbuf_Bak == '#') {	//单舵机模式	#000P1500T1000! 类似这种命令
				Usart1_Mode = 2;
			} else if(Sbuf_Bak == '{') {	//多舵机模式	{#000P1500T1000!#001P1500T1000!} 多个单舵机命令用大括号括起来
				Usart1_Mode = 3;
			} else if(Sbuf_Bak == '<') {	//保存动作组模式	<G0000#000P1500T1000!#001P1500T1000!B000!> 用尖括号括起来 带有组序号
				Usart1_Mode = 4;
			} 
			Buf_Index = 0;
		}
		
		Usart_Receive_Buf[Buf_Index++] = Sbuf_Bak;
				
		if((Usart1_Mode == 4) && (Sbuf_Bak == '>')){
			Usart_Receive_Buf[Buf_Index] = '\0';
			Usart1_Get_Ok = 1;
		} else if((Usart1_Mode == 1) && (Sbuf_Bak == '!')){
			Usart_Receive_Buf[Buf_Index] = '\0';
			Usart1_Get_Ok = 1;
		} else if((Usart1_Mode == 2) && (Sbuf_Bak == '!')){
			Usart_Receive_Buf[Buf_Index] = '\0';
			Usart1_Get_Ok = 1;
		} else if((Usart1_Mode == 3) && (Sbuf_Bak == '}')){
			Usart_Receive_Buf[Buf_Index] = '\0';
			Usart1_Get_Ok = 1;
		}    

		if(Buf_Index >= UART_BUF_SIZE)Buf_Index = 0;

	}
	
	return 0;
}

/***********************************************
	函数名称：USART2_IRQHandler() 
	功能介绍：串口2收发中断处理函数
	函数参数：无
	返回值：  无
 ***********************************************/
int USART2_IRQHandler(void) {
	u8 Sbuf_Bak;
	static u16 Buf_Index = 0;
	if(USART_GetFlagStatus(USART2,USART_IT_RXNE)==SET) {
		USART_ClearITPendingBit(USART2, USART_IT_RXNE);
		Sbuf_Bak = USART_ReceiveData(USART2);

		// 新增：接收ESP32视觉单字节指令（0x01=齿轮, 0x02=螺母）
		// 注意：这些单字节不进入原有协议缓冲区，直接存入rx_cmd
		if(Sbuf_Bak == 0x01 || Sbuf_Bak == 0x02) {
			rx_cmd = Sbuf_Bak;
		}

		// 原有协议处理保持不变（$CMD! / #000P1500T1000! / {...} / <...>）
		if(Sbuf_Bak == '<') {
			Usart1_Mode = 4;
			Buf_Index = 0;
		}else if(Usart1_Mode == 0) {
			if(Sbuf_Bak == '$') {
				Usart1_Mode = 1;
			} else if(Sbuf_Bak == '#') {
				Usart1_Mode = 2;
			} else if(Sbuf_Bak == '{') {
				Usart1_Mode = 3;
			} else if(Sbuf_Bak == '<') {
				Usart1_Mode = 4;
			}
			Buf_Index = 0;
		}

		Usart_Receive_Buf[Buf_Index++] = Sbuf_Bak;

		if((Usart1_Mode == 4) && (Sbuf_Bak == '>')){
			Usart_Receive_Buf[Buf_Index] = '\0';
			Usart1_Get_Ok = 1;
		} else if((Usart1_Mode == 1) && (Sbuf_Bak == '!')){
			Usart_Receive_Buf[Buf_Index] = '\0';
			Usart1_Get_Ok = 1;
		} else if((Usart1_Mode == 2) && (Sbuf_Bak == '!')){
			Usart_Receive_Buf[Buf_Index] = '\0';
			Usart1_Get_Ok = 1;
		} else if((Usart1_Mode == 3) && (Sbuf_Bak == '}')){
			Usart_Receive_Buf[Buf_Index] = '\0';
			Usart1_Get_Ok = 1;
		}

		if(Buf_Index >= UART_BUF_SIZE)Buf_Index = 0;

	}
	return 0;
}

/***********************************************
	函数名称：USART3_IRQHandler() 
	功能介绍：串口3收发中断处理函数
	函数参数：无
	返回值：  无
 ***********************************************/
int USART3_IRQHandler(void) { 
	u8 Sbuf_Bak;
	static u16 Buf_Index = 0;
	if(USART_GetFlagStatus(USART3,USART_IT_RXNE)==SET) {
		USART_ClearITPendingBit(USART3, USART_IT_RXNE);	
		Sbuf_Bak = USART_ReceiveData(USART3);
		if(Usart1_Get_Ok)return 0;
		if(Sbuf_Bak == '<') {
			Usart1_Mode = 4;
			Buf_Index = 0;
		}else if(Usart1_Mode == 0) {
			if(Sbuf_Bak == '$') {		
				Usart1_Mode = 1;
			} else if(Sbuf_Bak == '#') {
				Usart1_Mode = 2;
			} else if(Sbuf_Bak == '{') {
				Usart1_Mode = 3;
			} else if(Sbuf_Bak == '<') {
				Usart1_Mode = 4;
			} 
			Buf_Index = 0;
		}
		
		Usart_Receive_Buf[Buf_Index++] = Sbuf_Bak;
				
		if((Usart1_Mode == 4) && (Sbuf_Bak == '>')){
			Usart_Receive_Buf[Buf_Index] = '\0';
			Usart1_Get_Ok = 1;
		} else if((Usart1_Mode == 1) && (Sbuf_Bak == '!')){
			Usart_Receive_Buf[Buf_Index] = '\0';
			Usart1_Get_Ok = 1;
		} else if((Usart1_Mode == 2) && (Sbuf_Bak == '!')){
			Usart_Receive_Buf[Buf_Index] = '\0';
			Usart1_Get_Ok = 1;
		} else if((Usart1_Mode == 3) && (Sbuf_Bak == '}')){
			Usart_Receive_Buf[Buf_Index] = '\0';
			Usart1_Get_Ok = 1;
		}    

		if(Buf_Index >= UART_BUF_SIZE)Buf_Index = 0;
			
	}
//    if(Usart1_Get_Ok == 1){ //如果总线上有回传的数据，当数据接收完成之后，通过串口1发送返回至上位机
//        Usart1_Send_Str(Usart_Receive_Buf);
//    }
	return 0;
}

/***********************************************
	函数名称：Bususart_Send_Str() 
	功能介绍：总线（串口3）发送字符串
	函数参数：
	返回值：  无
 ***********************************************/
void Bususart_Send_Str(u8 *str) {
	Usart1_Get_Ok  = 1;
	Usart3_Send_Str(str);
    //Delay_ms(100);
    Usart1_Send_Str(str);    
	Usart1_Get_Ok = 0;
}
