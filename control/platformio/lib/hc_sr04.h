#ifndef __HC_SR04_H__
#define __HC_SR04_H__

#include <Arduino.h>

#define TRIG_PIN  PB0
#define ECHO_PIN  PB1

void hc_sr04_init() {
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    digitalWrite(TRIG_PIN, LOW);
}

float hc_sr04_read() {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    long duration = pulseIn(ECHO_PIN, HIGH, 30000);
    if (duration == 0) return -1;
    return duration * 0.034 / 2.0;
}

#endif
