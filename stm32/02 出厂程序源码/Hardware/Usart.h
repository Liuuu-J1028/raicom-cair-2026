#ifndef __USART_H__
#define __USART_H__


#include "stm32f10x_usart.h"
#include "Global.h"

#define TB_USART_FLAG_ERR  0X0F
#define TB_USART_FLAG_RXNE 0X20
#define TB_USART_FLAG_TXE  0X80

#define Interrupt_Open() {__enable_irq();}

#define Usart1_Open() 	{USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);}
#define Usart1_Close() 	{USART_ITConfig(USART1, USART_IT_RXNE, DISABLE);}

#define Usart2_Open() 	{USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);}		
#define Usart2_Close() 	{USART_ITConfig(USART2, USART_IT_RXNE, DISABLE);}		

#define Usart3_Open() 	{USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);}		
#define Usart3_Close() 	{USART_ITConfig(USART3, USART_IT_RXNE, DISABLE);}		

void Usart_Init(void);
void Usart1_Init(u32 rate);
void Usart2_Init(u32 rate);
void Usart3_Init(u32 rate);

void Usart1_Send_Byte(u8 Data);
void Usart1_Send_nByte(u8 *Data, u16 size);
void Usart1_Send_Str(u8 *Data);

void Usart2_Send_Byte(u8 Data);
void Usart2_Send_nByte(u8 *Data, u16 size);
void Usart2_Send_Str(u8 *Data);

void Usart3_Send_Byte(u8 Data);
void Usart3_Send_nByte(u8 *Data, u16 size);
void Usart3_Send_Str(u8 *Data);

int Usart1_Interrupt(void);
int Usart2_Interrupt(void);
int Usart3_Interrupt(void);

void Bususart_Send_Str(u8 *str);

#endif

