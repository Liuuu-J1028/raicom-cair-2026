/**
 * @file    walker.h
 * @brief   RAICOM 比赛机器人 - 行走控制模块（配置与接口）
 *
 * 【修改指南】拿到车后改下面「引脚配置」部分即可
 */

#ifndef __WALKER_H__
#define __WALKER_H__

#include "stm32f1xx_hal.h"

/* ================================================================
 * 🔧 引脚配置（根据实物改这里！）
 * ================================================================ */

/* ---- 左电机方向 ---- */
#define L_DIR1_PORT    GPIOA
#define L_DIR1_PIN     GPIO_PIN_4
#define L_DIR2_PORT    GPIOA
#define L_DIR2_PIN     GPIO_PIN_5

/* ---- 右电机方向 ---- */
#define R_DIR1_PORT    GPIOA
#define R_DIR1_PIN     GPIO_PIN_6
#define R_DIR2_PORT    GPIOA
#define R_DIR2_PIN     GPIO_PIN_7

/* ---- PWM 调速 ---- */
#define L_PWM_TIMER    htim2           // 定时器2
#define L_PWM_CHANNEL  TIM_CHANNEL_1   // 通道1 → PA0

#define R_PWM_TIMER    htim2           // 定时器2
#define R_PWM_CHANNEL  TIM_CHANNEL_2   // 通道2 → PA1

#define PWM_MAX        999             // ARR 值（0~999，即 1000 级调速）

/* ================================================================
 * ⏱️ 时序参数（实测后微调）
 * ================================================================ */

#define WALK_TIME_TO_PICKUP   1200    // 中转区→识别区（ms）
#define WALK_TIME_BACK         800    // 后退时间（ms）
#define WALK_TIME_TURN_90      600    // 90°转向（ms）
#define WALK_TIME_TO_DELIVERY 1500    // 转向后→配送区（ms）
#define WALK_DEFAULT_SPEED     650    // 默认 PWM 占空比（0~999）

/* ================================================================
 * 📋 对外接口
 * ================================================================ */

void walk_init(void);                          // 初始化
void walk_stop(void);                          // 停车
void walk_forward(void);                       // 前进
void walk_backward(void);                      // 后退
void walk_turn_left(void);                     // 左转（原地）
void walk_turn_right(void);                    // 右转（原地）
void walk_forward_ms(uint32_t ms);             // 前进 N 毫秒后自动停
void walk_backward_ms(uint32_t ms);            // 后退 N 毫秒
void walk_turn_left_ms(uint32_t ms);           // 左转 N 毫秒
void walk_turn_right_ms(uint32_t ms);          // 右转 N 毫秒
void walk_set_speed(uint16_t speed);           // 调速（0~999）
uint16_t walk_get_speed(void);                 // 查当前速度
void walk_do_transport(uint8_t direction);     // 执行一次完整搬运

#endif /* __WALKER_H__ */
