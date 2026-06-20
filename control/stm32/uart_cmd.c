/**
 * @file    uart_cmd.c
 * @brief   RAICOM - STM32 UART 指令接收（中断方式）
 *
 * ESP32 串口发 1 字节 → STM32 中断接收 → 存全局变量
 * CubeIDE 里需打开 USART1 的 NVIC 中断
 * stm32f1xx_it.c 的 USART1_IRQHandler 里调 uart_rx_callback()
 */

#include "uart_cmd.h"
#include <string.h>
#include <stdio.h>

static volatile uint8_t g_cmd = CMD_NONE;
static uint8_t          g_rx_buf;

void uart_cmd_init(void) {
    g_cmd = CMD_NONE;
    HAL_UART_Receive_IT(&CMD_UART, &g_rx_buf, 1);
}

void uart_rx_callback(uint8_t byte) {
    if (byte == CMD_SCREW || byte == CMD_NUT) {
        g_cmd = byte;
    }
    HAL_UART_Receive_IT(&CMD_UART, &g_rx_buf, 1);
}

uint8_t uart_cmd_get(void) {
    uint8_t cmd = g_cmd;
    g_cmd = CMD_NONE;
    return cmd;
}

uint8_t uart_cmd_peek(void) {
    return g_cmd;
}

void uart_cmd_clear(void) {
    g_cmd = CMD_NONE;
}

void uart_debug(const char *msg) {
    HAL_UART_Transmit(&DBG_UART, (uint8_t *)msg, strlen(msg), 100);
}
