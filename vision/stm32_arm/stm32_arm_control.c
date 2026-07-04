/**
 * STM32 机械臂抓取控制
 * 
 * 功能:
 * - 通过USART1接收PC发送的抓取命令 (JSON格式)
 * - 控制多个舵机完成机械臂抓取动作
 * - 已有机轮电机控制保持不变
 * 
 * 接线:
 * - USART1: PA9(TX), PA10(RX)  ← 连接PC串口
 * - 舵机1 (底座旋转): TIM通道 PA0
 * - 舵机2 (大臂): PA1
 * - 舵机3 (小臂): PA2
 * - 舵机4 (夹爪): PA3
 * 
 * 需要使用 ArduinoJson 库 或手动解析JSON
 */

#include "stm32f1xx_hal.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// ========== 舵机配置 ==========
#define SERVO1_CHANNEL   TIM_CHANNEL_1   // 底座旋转
#define SERVO2_CHANNEL   TIM_CHANNEL_2   // 大臂
#define SERVO3_CHANNEL   TIM_CHANNEL_3   // 小臂
#define SERVO4_CHANNEL   TIM_CHANNEL_4   // 夹爪

// 舵机角度范围
#define SERVO_MIN_PULSE   500    // 0度 (0.5ms)
#define SERVO_MAX_PULSE   2500   // 180度 (2.5ms)
#define SERVO_PERIOD      20000  // 20ms周期

// 预设抓取位置
#define GRAB_ANGLE_BASE     90   // 底座角度
#define GRAB_ANGLE_ARM1     60   // 大臂角度
#define GRAB_ANGLE_ARM2     120  // 小臂角度
#define GRAB_ANGLE_GRIP_OPEN  80  // 夹爪张开
#define GRAB_ANGLE_GRIP_CLOSE 40  // 夹爪闭合

// ========== 串口接收缓冲 ==========
#define RX_BUF_SIZE 256
uint8_t rx_buf[RX_BUF_SIZE];
volatile uint8_t rx_index = 0;
volatile uint8_t rx_complete = 0;

// 舵机控制句柄
TIM_HandleTypeDef *servo_tim;

// ========== 舵机角度设置 ==========
void Servo_SetAngle(TIM_HandleTypeDef *htim, uint32_t channel, uint8_t angle)
{
    // 将角度(0-180)转换为脉宽(500-2500us)
    uint32_t pulse = SERVO_MIN_PULSE + (uint32_t)(angle * (SERVO_MAX_PULSE - SERVO_MIN_PULSE) / 180);
    __HAL_TIM_SET_COMPARE(htim, channel, pulse);
}

// 机械臂回到初始位置
void Arm_HomePosition(void)
{
    Servo_SetAngle(servo_tim, SERVO1_CHANNEL, 90);
    Servo_SetAngle(servo_tim, SERVO2_CHANNEL, 90);
    Servo_SetAngle(servo_tim, SERVO3_CHANNEL, 90);
    Servo_SetAngle(servo_tim, SERVO4_CHANNEL, GRAB_ANGLE_GRIP_OPEN);
    HAL_Delay(500);
}

// 机械臂抓取动作
void Arm_Grab(uint8_t x, uint8_t y, const char *object_class)
{
    // 根据物体位置(x,y)计算底座旋转角度
    // 假设：画面中心(320,240)对应底座90度
    uint8_t base_angle = 90 + (int16_t)(x - 320) * 30 / 320;
    if (base_angle > 180) base_angle = 180;
    if (base_angle < 0)   base_angle = 0;
    
    // 根据物体y坐标计算大臂角度
    uint8_t arm_angle = 40 + (uint16_t)(y - 100) * 40 / 380;
    if (arm_angle > 180) arm_angle = 180;
    
    // 1. 张开夹爪
    Servo_SetAngle(servo_tim, SERVO4_CHANNEL, GRAB_ANGLE_GRIP_OPEN);
    HAL_Delay(300);
    
    // 2. 旋转底座对准目标
    Servo_SetAngle(servo_tim, SERVO1_CHANNEL, base_angle);
    HAL_Delay(500);
    
    // 3. 放下大臂
    Servo_SetAngle(servo_tim, SERVO2_CHANNEL, arm_angle);
    HAL_Delay(500);
    
    // 4. 放下小臂
    Servo_SetAngle(servo_tim, SERVO3_CHANNEL, 150);
    HAL_Delay(300);
    
    // 5. 闭合夹爪抓取
    Servo_SetAngle(servo_tim, SERVO4_CHANNEL, GRAB_ANGLE_GRIP_CLOSE);
    HAL_Delay(500);
    
    // 6. 抬起机械臂
    Servo_SetAngle(servo_tim, SERVO3_CHANNEL, 90);
    HAL_Delay(300);
    Servo_SetAngle(servo_tim, SERVO2_CHANNEL, 90);
    HAL_Delay(500);
    
    // 7. 旋转到放置位置
    Servo_SetAngle(servo_tim, SERVO1_CHANNEL, 0);
    HAL_Delay(500);
    
    // 8. 放下物体
    Servo_SetAngle(servo_tim, SERVO2_CHANNEL, 100);
    HAL_Delay(500);
    Servo_SetAngle(servo_tim, SERVO4_CHANNEL, GRAB_ANGLE_GRIP_OPEN);
    HAL_Delay(300);
    
    // 9. 归位
    Arm_HomePosition();
}

// 简单JSON解析器 (避免引入大库)
int json_get_int(const char *json, const char *key)
{
    char search[64];
    snprintf(search, sizeof(search), "\"%s\":", key);
    char *pos = strstr(json, search);
    if (pos) {
        pos += strlen(search);
        return atoi(pos);
    }
    return -1;
}

void json_get_string(const char *json, const char *key, char *out, int max_len)
{
    char search[64];
    snprintf(search, sizeof(search), "\"%s\":\"", key);
    char *pos = strstr(json, search);
    if (pos) {
        pos += strlen(search);
        char *end = strchr(pos, '"');
        if (end) {
            int len = end - pos;
            if (len < max_len) {
                strncpy(out, pos, len);
                out[len] = '\0';
                return;
            }
        }
    }
    out[0] = '\0';
}

// 处理接收到的命令
void ProcessCommand(const char *json_str)
{
    // 提取cmd字段
    char cmd[32] = {0};
    json_get_string(json_str, "cmd", cmd, sizeof(cmd));
    
    if (strcmp(cmd, "grab") == 0) {
        int x = json_get_int(json_str, "x");
        int y = json_get_int(json_str, "y");
        char object_class[32] = {0};
        json_get_string(json_str, "class", object_class, sizeof(object_class));
        
        printf("抓取命令: %s 位置(%d,%d)\r\n", object_class, x, y);
        Arm_Grab(x, y, object_class);
        printf("抓取完成!\r\n");
    }
    else if (strcmp(cmd, "release") == 0) {
        printf("释放物体\r\n");
        Servo_SetAngle(servo_tim, SERVO4_CHANNEL, GRAB_ANGLE_GRIP_OPEN);
    }
    else if (strcmp(cmd, "home") == 0) {
        printf("机械臂归位\r\n");
        Arm_HomePosition();
    }
    else {
        printf("未知命令: %s\r\n", cmd);
    }
}

// ========== 初始化 ==========
void SystemClock_Config(void);
void MX_GPIO_Init(void);
void MX_USART1_UART_Init(void);
void MX_TIM2_Init(void);  // 舵机PWM定时器

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();
    MX_TIM2_Init();
    
    servo_tim = &htim2;
    
    // 启动舵机PWM
    HAL_TIM_PWM_Start(servo_tim, SERVO1_CHANNEL);
    HAL_TIM_PWM_Start(servo_tim, SERVO2_CHANNEL);
    HAL_TIM_PWM_Start(servo_tim, SERVO3_CHANNEL);
    HAL_TIM_PWM_Start(servo_tim, SERVO4_CHANNEL);
    
    // 使能串口中断接收
    HAL_UART_Receive_IT(&huart1, &rx_buf[rx_index], 1);
    
    printf("\r\n=== STM32 机械臂抓取系统 ===\r\n");
    printf("等待PC命令...\r\n");
    
    // 初始归位
    Arm_HomePosition();
    
    while (1)
    {
        if (rx_complete) {
            rx_buf[rx_index] = '\0';
            printf("收到PC命令: %s\r\n", (char*)rx_buf);
            
            ProcessCommand((char*)rx_buf);
            
            // 重置接收缓冲
            rx_index = 0;
            rx_complete = 0;
            memset(rx_buf, 0, RX_BUF_SIZE);
            
            // 重新启动接收
            HAL_UART_Receive_IT(&huart1, &rx_buf[rx_index], 1);
        }
    }
}

// ========== 串口中断回调 ==========
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        if (rx_buf[rx_index] == '\n') {
            rx_complete = 1;
        } else {
            rx_index++;
            if (rx_index >= RX_BUF_SIZE - 1) {
                rx_index = 0;  // 溢出保护
            }
            HAL_UART_Receive_IT(&huart1, &rx_buf[rx_index], 1);
        }
    }
}

// ========== 舵机PWM定时器初始化 ==========
void MX_TIM2_Init(void)
{
    TIM_OC_InitTypeDef sConfigOC = {0};
    
    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 72 - 1;       // 72MHz / 72 = 1MHz
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = SERVO_PERIOD - 1; // 20000 → 20ms
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_PWM_Init(&htim2);
    
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 1500;  // 初始90度位置
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    
    HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, SERVO1_CHANNEL);
    HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, SERVO2_CHANNEL);
    HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, SERVO3_CHANNEL);
    HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, SERVO4_CHANNEL);
}

// ========== 串口初始化 (printf重定向到USART1) ==========
void MX_USART1_UART_Init(void)
{
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    HAL_UART_Init(&huart1);
}

// printf重定向到USART1
int fputc(int ch, FILE *f)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}
