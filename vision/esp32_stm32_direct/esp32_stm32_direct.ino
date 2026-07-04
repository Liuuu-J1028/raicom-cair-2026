/**
 * ESP32-CAM → STM32 直连识别（方案一：ESP32 本地识别 + UART2 单字节指令）
 *
 * 架构：
 *   ESP32-CAM 本地拍照 → 简单图像处理识别物体 → UART2 发 0x01/0x02 给 STM32
 *   无需 PC 参与，小车完全自主运行。
 *
 * 接线：
 *   ESP32-CAM GPIO17 (TX2) → STM32 PA3 (RX)
 *   ESP32-CAM GPIO16 (RX2) → STM32 PA2 (TX)
 *   ESP32-CAM GND          → STM32 GND
 *
 * 重要提示：
 *   1. 本示例使用灰度图 + 左右半区亮度差作为占位识别逻辑。
 *      实际比赛中请根据齿轮/螺母的真实颜色、形状、位置替换为有效算法。
 *   2. 若需更高精度，可考虑：
 *      - 颜色阈值分割（若齿轮/螺母颜色差异明显）
 *      - TFLite Micro 轻量模型（需把 YOLO 模型转换并量化）
 *      - 更换为 K210 / ESP32-S3 等带 AI 加速的模块
 *   3. 大多数 ESP32-CAM 板载 4MB PSRAM，本代码使用 PSRAM 存放图像。
 *      如果你的模块没有 PSRAM，请把 fb_location 改为 CAMERA_FB_IN_DRAM
 *      并把 frame_size 改为 FRAMESIZE_QQVGA (160x120)。
 */

#include "esp_camera.h"
#include <HardwareSerial.h>

HardwareSerial STM32Serial(2);

// ========== OV2640 引脚 (ESP32-CAM 标准接线) ==========
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

#define LED_PIN            4

// ========== 识别参数（TODO: 根据实际调试修改）==========
// 物体类别定义，与 STM32 的 rx_cmd 对应
#define OBJ_NONE   0
#define OBJ_GEAR   1   // 0x01 → STM32 执行齿轮抓取
#define OBJ_BOLT   2   // 0x02 → STM32 执行螺母抓取

// 亮度差阈值：左右半区平均亮度差超过此值才认为识别到物体
// 需要根据实际光照和物体颜色调整
#define BRIGHTNESS_DIFF_THRESHOLD  20

// 发送防抖间隔 (ms)：避免同一物体被连续多次识别
#define SEND_COOLDOWN_MS           1500

// ========== 全局状态 ==========
static uint32_t lastSendTime = 0;

void setup() {
  Serial.begin(115200);
  // UART2: RX=GPIO16, TX=GPIO17, 波特率 115200（与 STM32 USART2 对齐）
  STM32Serial.begin(115200, SERIAL_8N1, 16, 17);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.println("\n=================================");
  Serial.println("ESP32-CAM → STM32 直连识别");
  Serial.println("=================================");

  if (!initCamera()) {
    Serial.println("[ERROR] 摄像头初始化失败，5秒后重启...");
    delay(5000);
    ESP.restart();
  }

  Serial.println("[OK] 系统就绪，开始识别...");
  Serial.println();
}

bool initCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;

  // 使用灰度图：节省内存，省去 JPEG 解码步骤
  config.pixel_format = PIXFORMAT_GRAYSCALE;
  // QVGA 320x240，平衡精度与处理速度
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  // 使用 PSRAM 存放图像帧（需要 ESP32-CAM 带 PSRAM，大多数都有）
  config.fb_location = CAMERA_FB_IN_PSRAM;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("[ERROR] 摄像头初始化失败: 0x%x\n", err);
    return false;
  }

  // 调整传感器参数，使图像更稳定
  sensor_t *s = esp_camera_sensor_get();
  s->set_brightness(s, 0);     // 亮度: -2 ~ 2
  s->set_contrast(s, 0);       // 对比度: -2 ~ 2
  s->set_saturation(s, -2);    // 饱和度: 灰度模式下无效，设为最低
  s->set_whitebal(s, 0);       // 关闭自动白平衡（灰度模式下无意义）
  s->set_exposure_ctrl(s, 1);  // 开启自动曝光

  Serial.printf("[OK] 摄像头初始化完成: %dx%d 灰度\n", config.frame_size == FRAMESIZE_QVGA ? 320 : 0, 240);
  return true;
}

/**
 * @brief 简单识别逻辑（占位实现）
 *
 * 当前实现：统计画面左右半区的平均亮度，根据亮度差判断物体位置。
 * 这只是一个示例框架，实际比赛中必须替换为可靠算法。
 *
 * 建议替换方案：
 * 1. 若齿轮/螺母颜色差异大 → 用颜色阈值分割（需改回 RGB 格式）
 * 2. 若形状差异大 → 用边缘检测 + 轮廓圆度/多边形分析
 * 3. 若已训练 TFLite 模型 → 把模型放 SD 卡或 Flash，调用 interpreter->Invoke()
 */
int detectObject(uint8_t *img, int width, int height) {
  int mid = width / 2;
  uint32_t leftSum = 0, rightSum = 0;

  // 统计左右半区平均亮度
  for (int y = 0; y < height; y++) {
    uint8_t *row = img + y * width;
    for (int x = 0; x < mid; x++) {
      leftSum += row[x];
    }
    for (int x = mid; x < width; x++) {
      rightSum += row[x];
    }
  }

  int leftAvg = leftSum / (height * mid);
  int rightAvg = rightSum / (height * mid);
  int diff = leftAvg - rightAvg;

  // TODO: 根据实际物体特征重写此判断逻辑
  // 示例：假设齿轮在画面左侧且亮度较高
  if (diff > BRIGHTNESS_DIFF_THRESHOLD) {
    return OBJ_GEAR;  // 左边更亮 → 认为是齿轮
  }
  // 示例：假设螺母在画面右侧
  else if (-diff > BRIGHTNESS_DIFF_THRESHOLD) {
    return OBJ_BOLT;  // 右边更亮 → 认为是螺母
  }

  return OBJ_NONE;
}

void sendToSTM32(uint8_t cmdByte) {
  STM32Serial.write(cmdByte);

  digitalWrite(LED_PIN, HIGH);
  delay(100);
  digitalWrite(LED_PIN, LOW);

  if (cmdByte == 0x01) {
    Serial.println("[TX] 0x01 → STM32 (齿轮)");
  } else if (cmdByte == 0x02) {
    Serial.println("[TX] 0x02 → STM32 (螺母)");
  }
}

void loop() {
  // 拍照
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("[WARN] 拍照失败，跳过本次");
    delay(100);
    return;
  }

  int width = fb->width;
  int height = fb->height;

  // 识别
  int result = detectObject(fb->buf, width, height);

  esp_camera_fb_return(fb);

  // 发送结果（带防抖）
  uint32_t now = millis();
  if (result != OBJ_NONE && (now - lastSendTime) > SEND_COOLDOWN_MS) {
    if (result == OBJ_GEAR) {
      sendToSTM32(0x01);
    } else if (result == OBJ_BOLT) {
      sendToSTM32(0x02);
    }
    lastSendTime = now;
  }
  else if (result == OBJ_NONE) {
    // 调试用：每 2 秒打印一次左右亮度，方便观察阈值
    static uint32_t lastDebug = 0;
    if (now - lastDebug > 2000) {
      // 这里不再重新计算，仅示意；如需详细调试可重构 detectObject 返回更多信息
      Serial.println("[DBG] 未识别到目标");
      lastDebug = now;
    }
  }

  // 帧率约 3~5 FPS（受限于图像处理和串口）
  delay(200);
}
