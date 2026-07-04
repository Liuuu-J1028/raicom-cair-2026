/*
 * ESP32端 - 向PC发送检测指令并接收结果
 * 
 * 接线说明:
 * - ESP32通过Micro USB连接到电脑
 * - 如果需要控制舵机: 舵机信号线接 GPIO 5
 */

#include <ArduinoJson.h>

// 舵机控制引脚 (如有需要)
#define SERVO_PIN 5
#define LED_PIN 2  // ESP32内置LED

String receivedData = "";
bool detectionComplete = false;

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  
  Serial.println("ESP32 就绪");
  Serial.println("发送指令:");
  Serial.println("  '1' - 请求检测");
  Serial.println("  '2' - 检查连接");
  
  digitalWrite(LED_PIN, HIGH);
  delay(500);
  digitalWrite(LED_PIN, LOW);
}

void loop() {
  // 检查PC返回的检测结果
  if (Serial.available()) {
    receivedData = Serial.readStringUntil('\n');
    receivedData.trim();
    
    if (receivedData.length() > 0) {
      Serial.println("收到PC响应: " + receivedData);
      parseDetectionResult(receivedData);
      detectionComplete = true;
    }
  }
  
  // 如果有按键输入，发送检测指令
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    
    if (cmd == "1") {
      Serial.println("发送: DETECT");
      Serial.println("DETECT");
      digitalWrite(LED_PIN, HIGH);
    }
    else if (cmd == "2") {
      Serial.println("发送: CHECK");
      Serial.println("CHECK");
    }
  }
  
  delay(50);
}

void parseDetectionResult(String json) {
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, json);
  
  if (error) {
    Serial.print("JSON解析失败: ");
    Serial.println(error.c_str());
    return;
  }
  
  const char* status = doc["status"];
  int count = doc.containsKey("count") ? doc["count"] : 0;
  
  Serial.println("=== 检测结果 ===");
  Serial.print("状态: ");
  Serial.println(status);
  Serial.print("检测到物体数: ");
  Serial.println(count);
  
  if (count > 0) {
    JsonArray objects = doc["objects"];
    for (JsonObject obj : objects) {
      const char* className = obj["class"];
      float confidence = obj["confidence"];
      
      Serial.print("  物体: ");
      Serial.print(className);
      Serial.print("  置信度: ");
      Serial.println(confidence);
      
      // 根据检测结果控制机器人
      if (strcmp(className, "gear") == 0) {
        Serial.println("  -> 检测到齿轮!");
        // 例如: 转动舵机到某个角度
        // myservo.write(90);
      }
      else if (strcmp(className, "bolt") == 0) {
        Serial.println("  -> 检测到螺栓!");
        // 例如: 转动舵机到另一个角度
        // myservo.write(180);
      }
    }
  }
  
  digitalWrite(LED_PIN, LOW);
}
