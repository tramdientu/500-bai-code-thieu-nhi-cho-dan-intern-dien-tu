#pragma once
#include <Arduino.h>
#include <HomeSpan.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ir_Fujitsu.h>
#include "DHT.h"
#include "config.h"

// ===== Object dùng chung =====
extern IRsend       irsend;
extern IRFujitsuAC  fujitsuAC;
extern DHT          dht;

// ===== Biến DHT toàn cục =====
extern float g_temp;
extern float g_hum;
extern unsigned long lastDHT;

// ===== RF Queue =====
extern RFCommand rfQueue[RF_QUEUE_SIZE];
extern volatile int rfHead;
extern volatile int rfTail;

// ===== Khai báo các hàm =====
void powerRFOn();
void powerRFOff();
void sendRFBitBang(unsigned long code, uint8_t bitLen, uint16_t pulseLen, uint8_t repeat);
void enqueueRF(unsigned long code, uint8_t bitLen, uint16_t pulseLen, uint8_t repeat = RF_REPEAT_TX);
bool dequeueRF(RFCommand &cmd);

void sendIR(uint32_t code);
void readDHT();
void addAccessoryInfo(const char* name);
