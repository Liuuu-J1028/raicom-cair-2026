/*
 * ESP32-CAM 全无线方案
 * 拍照 → WiFi发PC(识别) → 收PC结果 → UART传STM32(抓取)
 * 
 * 接线:
 * ESP32-CAM GPIO12(TX) → STM32 PA10(RX)
 * ESP32-CAM GPIO13(RX) → STM32 PA9(TX)
 * ESP32-CAM GND       → STM32 GND
 */

#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <HardwareSerial.h>

// ========== WiFi ==========
const char* ssid = "你的WiFi名称";
const char* password = "你的WiFi密码";
const char* pc_server = "http://192.168.1.100:8080/detect";

// ========== STM32串口 (GPIO12=TX, GPIO13=RX) ==========
HardwareSerial STM32Serial(2);  // UART2

// ========== OV2640引脚 ==========
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

#define LED_PIN 4

void setup() {
  Serial.begin(115200);
  STM32Serial.begin(115200, SERIAL_8N1, 13, 12);  // RX=13, TX=12
  
  Serial.println("\n=== 小车全无线抓取系统 ===");
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  // WiFi
  WiFi.begin(ssid, password);
  Serial.print("WiFi");
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 30) {
    delay(500); Serial.print("."); retry++;
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(" 失败! 重启...");
    ESP.restart();
  }
  Serial.println("\nWiFi已连接! IP:" + WiFi.localIP().toString());
  
  // 摄像头
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM; config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM; config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM; config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM; config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM; config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM; config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM; config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM; config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_VGA;
  config.jpeg_quality = 15;
  config.fb_count = 1;
  
  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("摄像头失败!"); return;
  }
  
  sensor_t* s = esp_camera_sensor_get();
  s->set_brightness(s, 1);
  s->set_contrast(s, 1);
  s->set_saturation(s, 0);
  s->set_whitebal(s, 1);
  s->set_awb_gain(s, 1);
  
  Serial.println("就绪! 输入'1'触发抓取");
}

void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();
    if (cmd == '1') {
      doDetection();
    }
  }
  delay(10);
}

void doDetection() {
  digitalWrite(LED_PIN, HIGH);
  
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("拍照失败!");
    digitalWrite(LED_PIN, LOW);
    return;
  }
  
  Serial.printf("拍照: %d bytes → PC...\n", fb->len);
  
  HTTPClient http;
  http.begin(pc_server);
  http.addHeader("Content-Type", "image/jpeg");
  http.setTimeout(5000);
  
  int httpCode = http.POST(fb->buf, fb->len);
  esp_camera_fb_return(fb);
  
  if (httpCode == 200) {
    String response = http.getString();
    Serial.println("PC结果: " + response);
    
    // 把PC返回的JSON结果直接转发给STM32
    // PC返回格式: {"status":"ok","count":2,"objects":[{"class":"gear",...}]}
    
    if (response.indexOf("\"gear\"") > 0 || response.indexOf("\"bolt\"") > 0) {
      // 构造STM32命令 — 提取第一个物体的坐标和类别
      String stm32Cmd = buildGrabCommand(response);
      STM32Serial.println(stm32Cmd);
      Serial.println("→ STM32: " + stm32Cmd);
    } else {
      Serial.println("未检测到物体");
    }
  } else {
    Serial.printf("HTTP失败: %d\n", httpCode);
  }
  
  http.end();
  digitalWrite(LED_PIN, LOW);
}

// 从PC返回的JSON中提取坐标，构造STM32抓取命令
String buildGrabCommand(String json) {
  int x = 160, y = 200;
  String objClass = "gear";
  
  // 简易提取 (无需JSON库)
  int cx = json.indexOf("\"x_center\":");
  int cy = json.indexOf("\"y_center\":");
  int cc = json.indexOf("\"class\":\"");
  
  if (cx > 0) x = json.substring(cx + 11).toInt();
  if (cy > 0) y = json.substring(cy + 11).toInt();
  if (cc > 0) {
    int end = json.indexOf("\"", cc + 9);
    if (end > cc) objClass = json.substring(cc + 9, end);
  }
  
  return "{\"cmd\":\"grab\",\"class\":\"" + objClass + "\",\"x\":" + String(x) + ",\"y\":" + String(y) + "}";
}
