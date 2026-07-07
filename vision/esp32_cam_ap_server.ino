#include <WiFi.h>
#include "esp_camera.h"

  // ========== OV2640 引脚（和你现在接的一样）==========
  #define PWDN_GPIO_NUM     -1
  #define RESET_GPIO_NUM     2
  #define XCLK_GPIO_NUM     -1
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

  // WiFi 热点配置
  const char* ssid = "ESP32-CAM";
  const char* password = "12345678";

  WiFiServer server(80);

  void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("Starting...");

    // 1. 初始化摄像头（改成 JPEG 格式，方便浏览器显示）
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;

    config.xclk_freq_hz = 24000000;
    config.frame_size = FRAMESIZE_QVGA;      // 320x240
    config.pixel_format = PIXFORMAT_JPEG;    // ← 改成 JPEG，浏览器能直接显示
    config.fb_location = CAMERA_FB_IN_DRAM;
    config.jpeg_quality = 10;                // 10=质量较好，0-63可调
    config.fb_count = 1;
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;

    if (esp_camera_init(&config) != ESP_OK) {
      Serial.println("Camera FAILED");
      return;
    }
    Serial.println("Camera OK");

    // 2. 启动 WiFi 热点
    WiFi.softAP(ssid, password);
    IPAddress IP = WiFi.softAPIP();
    Serial.print("WiFi Hotspot Started! Connect to: ");
    Serial.println(ssid);
    Serial.print("Password: ");
    Serial.println(password);
    Serial.print("Then open browser: http://");
    Serial.println(IP);  // 通常是 192.168.4.1

    server.begin();
  }

  void loop() {
    WiFiClient client = server.available();
    if (!client) return;

    String req = client.readStringUntil('\r');
    Serial.println(req);
    client.flush();

    // 访问 /capture 返回一帧 JPEG 图片
    if (req.indexOf("/capture") != -1) {
      camera_fb_t *fb = esp_camera_fb_get();
      if (!fb) {
        client.println("HTTP/1.1 500 Internal Server Error");
        client.stop();
        return;
      }

      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: image/jpeg");
      client.println("Connection: close");
      client.print("Content-Length: ");
      client.println(fb->len);
      client.println();
      client.write(fb->buf, fb->len);
      esp_camera_fb_return(fb);
    }
    // 访问根路径返回自动刷新网页
    else {
      String html = "<!DOCTYPE html><html><head>";
      html += "<meta charset='UTF-8'>";
      html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
      html += "<title>ESP32 Camera</title></head><body style='text-align:center;'>";
      html += "<h2>ESP32 Camera</h2>";
      html += "<img id='cam' src='/capture' style='width:100%; max-width:640px; border:1px solid #ccc;'>";
      html += "<p>Auto refresh every 500ms</p>";
      html += "<script>";
      html += "setInterval(function(){";
      html += "document.getElementById('cam').src='/capture?t='+Date.now();";
      html += "},500);";
      html += "</script></body></html>";

      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/html");
      client.println("Connection: close");
      client.println();
      client.print(html);
    }

    client.stop();
  }