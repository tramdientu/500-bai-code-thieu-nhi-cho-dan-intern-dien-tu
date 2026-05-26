#include "devices.h"

// ========== SafeSwitch ==========
SafeSwitch::SafeSwitch(const char* name, uint32_t cooldown)
  : Service::Switch() {
  power = new Characteristic::On(false);
  lastState = false;
  devName = name;
  lockUntil = 0;
  cooldownMs = cooldown;
}

boolean SafeSwitch::update() {
  bool newVal = power->getNewVal();
  if (newVal == lastState) return true;

  if (millis() < lockUntil) {
    unsigned long remain = lockUntil - millis();
    Serial.printf("[LOCK] %s đang khóa! Còn %lums - Bỏ qua %s\n",
                  devName, remain, newVal ? "ON" : "OFF");
    power->setVal(lastState);
    return false;
  }

  lastState = newVal;
  onChange(newVal);
  lockUntil = millis() + cooldownMs;
  Serial.printf("[LOCK] %s -> %s | Khóa %lums\n",
                devName, newVal ? "ON" : "OFF", cooldownMs);
  return true;
}

// ========== RFDevice ==========
RFDevice::RFDevice(const char* name,
                   unsigned long onCode,
                   unsigned long offCode,
                   uint8_t  bits,
                   uint16_t pulse)
  : SafeSwitch(name, RF_COOLDOWN_MS) {
  codeOn   = onCode;
  codeOff  = offCode;
  bitLen   = bits;
  pulseLen = pulse;
}

void RFDevice::onChange(bool state) {
  unsigned long code = state ? codeOn : codeOff;
  enqueueRF(code, bitLen, pulseLen, RF_REPEAT_TX);
  Serial.printf("[RF] %s -> %s (%lu | %ub | %uµs)\n",
                devName, state ? "ON" : "OFF",
                code, bitLen, pulseLen);
}

// ========== DenLED1 ==========
DenLED1::DenLED1() : SafeSwitch("Den LED 1", IR_COOLDOWN_MS) {}

void DenLED1::onChange(bool state) {
  sendIR(state ? IR_BAT_DEN : IR_TAT_DEN);
}
