/**
 * @file    walker.c
 * @brief   RAICOM 比赛机器人 - 行走控制模块
 * @硬件    STM32F103C8T6 + L298N 电机驱动（双路）
 * @注意    无编码器，靠定时器延时估算距离
 *
 * 引脚配置在 walker.h 中统一管理，按实际接线改一行即可
 */

/* ================================================================
 * 1. INCLUDES
 * ================================================================ */
#include "walker.h"
#include "tim.h"       // 定时器 PWM（CubeIDE 自动生成）
#include "gpio.h"      // GPIO 初始化
#include "usart.h"     // 调试输出（可选）

/* ================================================================
 * 2. 全局变量
 * ================================================================ */
static uint16_t g_speed = WALK_DEFAULT_SPEED;  // 当前速度 (0~PWM_MAX)

/* 方向标记 */
static uint8_t  g_left_dir  = 0;   // 左电机方向: 0=停止, 1=前进, 2=后退
static uint8_t  g_right_dir = 0;   // 右电机方向

/* ================================================================
 * 3. 底层驱动
 * ================================================================ */

/** 设置左电机方向 */
static void walk_set_left_dir(uint8_t dir) {
    g_left_dir = dir;
    switch (dir) {
        case 0:  // 刹车（两输入同时拉低或同时拉高）
            HAL_GPIO_WritePin(L_DIR1_PORT, L_DIR1_PIN, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(L_DIR2_PORT, L_DIR2_PIN, GPIO_PIN_RESET);
            break;
        case 1:  // 前进: DIR1=高, DIR2=低
            HAL_GPIO_WritePin(L_DIR1_PORT, L_DIR1_PIN, GPIO_PIN_SET);
            HAL_GPIO_WritePin(L_DIR2_PORT, L_DIR2_PIN, GPIO_PIN_RESET);
            break;
        case 2:  // 后退: DIR1=低, DIR2=高
            HAL_GPIO_WritePin(L_DIR1_PORT, L_DIR1_PIN, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(L_DIR2_PORT, L_DIR2_PIN, GPIO_PIN_SET);
            break;
    }
}

/** 设置右电机方向 */
static void walk_set_right_dir(uint8_t dir) {
    g_right_dir = dir;
    switch (dir) {
        case 0:
            HAL_GPIO_WritePin(R_DIR1_PORT, R_DIR1_PIN, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(R_DIR2_PORT, R_DIR2_PIN, GPIO_PIN_RESET);
            break;
        case 1:
            HAL_GPIO_WritePin(R_DIR1_PORT, R_DIR1_PIN, GPIO_PIN_SET);
            HAL_GPIO_WritePin(R_DIR2_PORT, R_DIR2_PIN, GPIO_PIN_RESET);
            break;
        case 2:
            HAL_GPIO_WritePin(R_DIR1_PORT, R_DIR1_PIN, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(R_DIR2_PORT, R_DIR2_PIN, GPIO_PIN_SET);
            break;
    }
}

/** 设置左轮 PWM 占空比 */
static void walk_set_left_pwm(uint16_t duty) {
    __HAL_TIM_SET_COMPARE(&L_PWM_TIMER, L_PWM_CHANNEL, duty);
}

/** 设置右轮 PWM 占空比 */
static void walk_set_right_pwm(uint16_t duty) {
    __HAL_TIM_SET_COMPARE(&R_PWM_TIMER, R_PWM_CHANNEL, duty);
}

/* ================================================================
 * 4. 初始化
 * ================================================================ */

void walk_init(void) {
    walk_set_left_dir(0);
    walk_set_right_dir(0);

    HAL_TIM_PWM_Start(&L_PWM_TIMER, L_PWM_CHANNEL);
    HAL_TIM_PWM_Start(&R_PWM_TIMER, R_PWM_CHANNEL);

    walk_set_left_pwm(0);
    walk_set_right_pwm(0);

    g_speed = WALK_DEFAULT_SPEED;
}

/* ================================================================
 * 5. 基本动作
 * ================================================================ */

void walk_stop(void) {
    walk_set_left_pwm(0);
    walk_set_right_pwm(0);
    walk_set_left_dir(0);
    walk_set_right_dir(0);
}

void walk_forward(void) {
    walk_set_left_dir(1);
    walk_set_right_dir(1);
    walk_set_left_pwm(g_speed);
    walk_set_right_pwm(g_speed);
}

void walk_backward(void) {
    walk_set_left_dir(2);
    walk_set_right_dir(2);
    walk_set_left_pwm(g_speed);
    walk_set_right_pwm(g_speed);
}

void walk_turn_left(void) {
    walk_set_left_dir(2);
    walk_set_right_dir(1);
    walk_set_left_pwm(g_speed);
    walk_set_right_pwm(g_speed);
}

void walk_turn_right(void) {
    walk_set_left_dir(1);
    walk_set_right_dir(2);
    walk_set_left_pwm(g_speed);
    walk_set_right_pwm(g_speed);
}

/* ================================================================
 * 6. 延时行走（无编码器 → 时间估算距离）
 * ================================================================ */

void walk_forward_ms(uint32_t ms) {
    walk_forward();
    HAL_Delay(ms);
    walk_stop();
}

void walk_backward_ms(uint32_t ms) {
    walk_backward();
    HAL_Delay(ms);
    walk_stop();
}

void walk_turn_left_ms(uint32_t ms) {
    walk_turn_left();
    HAL_Delay(ms);
    walk_stop();
}

void walk_turn_right_ms(uint32_t ms) {
    walk_turn_right();
    HAL_Delay(ms);
    walk_stop();
}

/* ================================================================
 * 7. 速度调节
 * ================================================================ */

void walk_set_speed(uint16_t speed) {
    if (speed > PWM_MAX) speed = PWM_MAX;
    g_speed = speed;
}

uint16_t walk_get_speed(void) {
    return g_speed;
}

/* ================================================================
 * 8. 比赛动作序列（×8 轮循环基础单元）
 * ================================================================ */

void walk_do_transport(uint8_t direction) {
    walk_forward_ms(WALK_TIME_TO_PICKUP);
    HAL_Delay(500);
    walk_backward_ms(WALK_TIME_BACK);
    if (direction == 0) {
        walk_turn_left_ms(WALK_TIME_TURN_90);
    } else {
        walk_turn_right_ms(WALK_TIME_TURN_90);
    }
    walk_forward_ms(WALK_TIME_TO_DELIVERY);
    HAL_Delay(500);
    walk_backward_ms(WALK_TIME_BACK);
    if (direction == 0) {
        walk_turn_right_ms(WALK_TIME_TURN_90);
    } else {
        walk_turn_left_ms(WALK_TIME_TURN_90);
    }
}
