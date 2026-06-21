#include <Arduino.h>
#include "hc_sr04.h"

#define L_DIR1 PA4
#define L_DIR2 PA5
#define R_DIR1 PA6
#define R_DIR2 PA7
#define L_PWM  PA0
#define R_PWM  PA1

#define TIME_BACK         800
#define TIME_TURN_90      600
#define TIME_TO_DELIVERY 1500
#define SPEED_DEFAULT     160
#define STOP_DIST_CM        8

#define CMD_NONE  0x00
#define CMD_SCREW 0x01
#define CMD_NUT   0x02

uint8_t g_round = 0;
HardwareSerial &ESP32_SERIAL = Serial1;

void stop(); void forward(); void backward();
void turn_left(); void turn_right();
void forward_ms(uint32_t ms); void backward_ms(uint32_t ms);
void turn_left_ms(uint32_t ms); void turn_right_ms(uint32_t ms);
void forward_until_dist(float dist_cm);
void do_transport(uint8_t dir);
uint8_t read_cmd();

void setup() {
    pinMode(L_DIR1, OUTPUT); pinMode(L_DIR2, OUTPUT);
    pinMode(R_DIR1, OUTPUT); pinMode(R_DIR2, OUTPUT);
    pinMode(L_PWM, PWM); pinMode(R_PWM, PWM);
    stop();
    hc_sr04_init();
    ESP32_SERIAL.begin(115200);
    Serial.begin(115200);
    Serial.println("STM32 ready");
}

void stop() {
    analogWrite(L_PWM, 0); analogWrite(R_PWM, 0);
    digitalWrite(L_DIR1, LOW); digitalWrite(L_DIR2, LOW);
    digitalWrite(R_DIR1, LOW); digitalWrite(R_DIR2, LOW);
}
void forward() {
    digitalWrite(L_DIR1, HIGH); digitalWrite(L_DIR2, LOW);
    digitalWrite(R_DIR1, HIGH); digitalWrite(R_DIR2, LOW);
    analogWrite(L_PWM, SPEED_DEFAULT); analogWrite(R_PWM, SPEED_DEFAULT);
}
void backward() {
    digitalWrite(L_DIR1, LOW); digitalWrite(L_DIR2, HIGH);
    digitalWrite(R_DIR1, LOW); digitalWrite(R_DIR2, HIGH);
    analogWrite(L_PWM, SPEED_DEFAULT); analogWrite(R_PWM, SPEED_DEFAULT);
}
void turn_left() {
    digitalWrite(L_DIR1, LOW); digitalWrite(L_DIR2, HIGH);
    digitalWrite(R_DIR1, HIGH); digitalWrite(R_DIR2, LOW);
    analogWrite(L_PWM, SPEED_DEFAULT); analogWrite(R_PWM, SPEED_DEFAULT);
}
void turn_right() {
    digitalWrite(L_DIR1, HIGH); digitalWrite(L_DIR2, LOW);
    digitalWrite(R_DIR1, LOW); digitalWrite(R_DIR2, HIGH);
    analogWrite(L_PWM, SPEED_DEFAULT); analogWrite(R_PWM, SPEED_DEFAULT);
}
void forward_ms(uint32_t ms)   { forward();   delay(ms); stop(); }
void backward_ms(uint32_t ms)  { backward();  delay(ms); stop(); }
void turn_left_ms(uint32_t ms)  { turn_left();  delay(ms); stop(); }
void turn_right_ms(uint32_t ms) { turn_right(); delay(ms); stop(); }

void forward_until_dist(float dist_cm) {
    forward();
    while (1) {
        float d = hc_sr04_read();
        if (d > 0 && d <= dist_cm) break;
        delay(30);
    }
    stop();
}

void do_transport(uint8_t dir) {
    forward_until_dist(STOP_DIST_CM);
    delay(500);
    backward_ms(TIME_BACK);
    if (dir == 0) turn_left_ms(TIME_TURN_90);
    else          turn_right_ms(TIME_TURN_90);
    forward_ms(TIME_TO_DELIVERY);
    delay(500);
    backward_ms(TIME_BACK);
    if (dir == 0) turn_right_ms(TIME_TURN_90);
    else          turn_left_ms(TIME_TURN_90);
}

uint8_t read_cmd() {
    if (ESP32_SERIAL.available()) {
        uint8_t b = ESP32_SERIAL.read();
        if (b == CMD_SCREW || b == CMD_NUT) return b;
    }
    return CMD_NONE;
}

void loop() {
    uint8_t cmd = read_cmd();
    if (cmd == CMD_NONE) return;
    g_round++;
    Serial.print("Round "); Serial.print(g_round);
    Serial.print(" | Dist: "); Serial.println(hc_sr04_read());
    if (cmd == CMD_SCREW) {
        Serial.println("Screw -> Zone 1");
        do_transport(0);
    } else {
        Serial.println("Nut -> Zone 2");
        do_transport(1);
    }
    if (g_round >= 8) {
        Serial.println("All 8 done!");
        while (1);
    }
}
