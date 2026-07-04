/**
 * ESP32 → STM32 测试发送器（方案一）
 *
 * 用途：不依赖摄像头，通过串口监视器手动触发指令，验证 STM32 端接收和抓取动作。
 *
 * 接线：
 *   ESP32 GPIO17 (TX2) → STM32 PA3 (RX)
 *   ESP32 GPIO16 (RX2) → STM32 PA2 (TX)
 *   ESP32 GND          → STM32 GND
 *
 * 用法：
 *   1. 烧录本代码到任意 ESP32 开发板
 *   2. 打开 Arduino 串口监视器（115200 baud，换行符任意）
 *   3. 输入 '1' → 发送 0x01（齿轮），输入 '2' → 发送 0x02（螺母）
 *   4. 观察 STM32 蜂鸣器和机械臂响应
 */

#include <HardwareSerial.h>

HardwareSerial STM32Serial(2);

void setup() {
  Serial.begin(115200);
  // UART2: RX=GPIO16, TX=GPIO17, 波特率 115200
  STM32Serial.begin(115200, SERIAL_8N1, 16, 17);

  Serial.println("=================================");
  Serial.println("ESP32 → STM32 测试发送器");
  Serial.println("=================================");
  Serial.println("输入 '1' → 发送 0x01 (齿轮)");
  Serial.println("输入 '2' → 发送 0x02 (螺母)");
  Serial.println();
}

void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();

    if (cmd == '1') {
      STM32Serial.write(0x01);
      Serial.println("→ STM32: 0x01 (齿轮指令已发送)");
    }
    else if (cmd == '2') {
      STM32Serial.write(0x02);
      Serial.println("→ STM32: 0x02 (螺母指令已发送)");
    }
    else if (cmd == '\n' || cmd == '\r') {
      // 忽略换行符
    }
    else {
      Serial.println("未知指令，请输 '1' 或 '2'");
    }
  }
  delay(10);
}
