#ifndef __SENSOR_H__
#define __SENSOR_H__

#include "stm32f10x_conf.h"

#define Trig(x) GpioB_PIN_Set(0, x);
#define Echo() GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_2)

#define Sensor_Tracking_Left() GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0)
#define Sensor_Tracking_Right() GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_1)

//处理智能传感器功能
void Sensor_Init(void);

void Tracking_Gpio_Init(void);
void Ultrasonic_Gpio_Init(void);

void Distance_Following(void);
void Obstacle_Avoidance(void);
void Auto_Tracking(void);
void Tracking_Avoidance(void);
void Color_Sorting(void);

int Ultrasonic_Get_Data(void);
int Ultrasonic_Data_Handle(void); 

#endif
