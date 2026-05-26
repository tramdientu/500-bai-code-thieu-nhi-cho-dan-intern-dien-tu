#pragma once
#include <Arduino.h>

// ===== CONFIG PIN =====
#define IR_SEND_PIN   4
#define RF_SEND_PIN   26
#define RF_POWER_PIN  16     // Chân điều khiển MODULE MOSFET (cấp nguồn FS1000A)
#define DHTPIN        15
#define DHTTYPE       DHT22

// ===== CALIBRATION =====
const float TEMP_OFFSET = 0.0;
const float HUM_OFFSET  = 0.0;

// ===== RF CONFIG =====
#define RF_BIT_LEN              24
#define RF_REPEAT_TX            10
#define RF_GAP_MS               200
#define RF_POWER_ON_DELAY_MS    15
#define RF_POWER_OFF_DELAY_MS   20

#define PULSE_LED2        390
#define PULSE_LED3        318
#define PULSE_DESK        307
#define PULSE_SHELF       297

#define RF_LED2_ON   3258625
#define RF_LED2_OFF  3258627
#define RF_LED3_ON   14289
#define RF_LED3_OFF  14290
#define RF_DESK_ON   12205409
#define RF_DESK_OFF  12205410
#define RF_SHELF_ON  9407985
#define RF_SHELF_OFF 9407986

#define RF_COOLDOWN_MS    1500
#define IR_COOLDOWN_MS    500

// ===== IR CONFIG =====
#define IR_BAT_DEN  0xF7C03F
#define IR_TAT_DEN  0xF740BF

// ===== WIFI & PAIRING =====
#define WIFI_SSID      "" 
#define WIFI_PASSWORD  ""
#define PAIRING_CODE   "88886666"

// ===== RF QUEUE =====
#define RF_QUEUE_SIZE  16

// ===== STRUCT RFCommand =====
struct RFCommand {
  unsigned long code;
  uint8_t       bitLen;
  uint16_t      pulseLen;
  uint8_t       repeat;
};
