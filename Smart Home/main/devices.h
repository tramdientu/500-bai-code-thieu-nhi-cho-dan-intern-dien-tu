#pragma once
#include "globals.h"

// ========== BASE SWITCH ==========
struct SafeSwitch : Service::Switch {
  SpanCharacteristic *power;
  bool lastState;
  const char* devName;
  unsigned long lockUntil;
  uint32_t cooldownMs;

  SafeSwitch(const char* name, uint32_t cooldown = RF_COOLDOWN_MS);
  virtual void onChange(bool state) {}
  boolean update() override;
};

// ========== RF DEVICE ==========
struct RFDevice : SafeSwitch {
  unsigned long codeOn;
  unsigned long codeOff;
  uint8_t       bitLen;
  uint16_t      pulseLen;

  RFDevice(const char* name,
           unsigned long onCode,
           unsigned long offCode,
           uint8_t  bits  = RF_BIT_LEN,
           uint16_t pulse = 350);
  void onChange(bool state) override;
};

// ========== IR DEVICE ==========
struct DenLED1 : SafeSwitch {
  DenLED1();
  void onChange(bool state) override;
};

// ========== ĐIỀU HÒA FUJITSU ==========
struct DieuHoaFujitsu : Service::HeaterCooler {
  SpanCharacteristic *active, *currentState, *targetState;
  SpanCharacteristic *currentTemp, *coolTemp, *heatTemp;
  SpanCharacteristic *rotSpeed, *swingMode;
  unsigned long lastUpdate = 0;
  unsigned long acLockUntil = 0;
  bool acLastActive = false;

  DieuHoaFujitsu();
  boolean update() override;
  void loop() override;
};

// ========== SENSORS ==========
struct TempSensor : Service::TemperatureSensor {
  SpanCharacteristic *temp;
  float lastVal;
  unsigned long lastUpdate;

  TempSensor();
  void loop() override;
};

struct HumSensor : Service::HumiditySensor {
  SpanCharacteristic *hum;
  float lastVal;
  unsigned long lastUpdate;

  HumSensor();
  void loop() override;
};
