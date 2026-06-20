/**
 * @file    uart_cmd.h
 * @brief   RAICOM - UART 指令接收模块
 *
 * ESP32 → UART → STM32
 * 协议: 0x01=螺丝→配送区1, 0x02=螺母→配送区2
 */

#ifndef __UART_CMD_H__
#define __UART_CMD_H__

#include "stm32f1xx_hal.h"

#define CMD_UART        huart1
#define CMD_UART_IRQ    USART1_IRQn
#define DBG_UART        huart2

#define CMD_SCREW       0x01
#define CMD_NUT         0x02
#define CMD_NONE        0x00

void uart_cmd_init(void);
uint8_t uart_cmd_get(void);
uint8_t uart_cmd_peek(void);
void uart_cmd_clear(void);
void uart_rx_callback(uint8_t byte);
void uart_debug(const char *msg);

#endif
