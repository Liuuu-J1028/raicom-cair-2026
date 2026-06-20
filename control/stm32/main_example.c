/**
 * @file    main_example.c
 * @brief   RAICOM - STM32 主循环示例
 *
 * 等 ESP32 指令 → walk_do_transport() → 循环 ×8
 *
 * ⚠️ 这是参考示例！合并到你 CubeIDE 生成的 main.c 里使用
 */

#include "main.h"
#include "walker.h"
#include "uart_cmd.h"

static uint8_t g_round = 0;

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_TIM2_Init();
    MX_USART1_UART_Init();
    MX_USART2_UART_Init();

    walk_init();
    uart_cmd_init();
    uart_debug("STM32 ready\r\n");

    while (1) {
        uint8_t cmd = uart_cmd_get();
        if (cmd == CMD_NONE) { HAL_Delay(50); continue; }

        g_round++;
        if (cmd == CMD_SCREW) {
            uart_debug("Screw->Zone1\r\n");
            walk_do_transport(0);
        } else if (cmd == CMD_NUT) {
            uart_debug("Nut->Zone2\r\n");
            walk_do_transport(1);
        }

        if (g_round >= 8) {
            uart_debug("Done!\r\n");
            while (1);
        }
    }
}
