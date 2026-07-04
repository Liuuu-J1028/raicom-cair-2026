#ifndef __DELAY_H__
#define __DELAY_H__

#include "stm32f10x.h"                  // Device header
#include "Type.h"

void Delay_Init(void);
void Delay(u16 t);
void Delay_ns(u16 t);
void Delay_us(u16 t);
void Delay_ms(u16 t);

#endif
