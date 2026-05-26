#include "devices.h"

// ========== QUẢN LÝ NGUỒN FS1000A QUA MOSFET ==========
void powerRFOn() {
  digitalWrite(RF_POWER_PIN, HIGH);     // Bật MOSFET → cấp nguồn FS1000A
  delay(RF_POWER_ON_DELAY_MS);          // Đợi FS1000A khởi động ổn định
}

void powerRFOff() {
  digitalWrite(RF_SEND_PIN, LOW);       // Đảm bảo DATA về LOW trước
  delay(RF_POWER_OFF_DELAY_MS);         // Đợi tín hiệu phát hết
  digitalWrite(RF_POWER_PIN, LOW);      // Tắt MOSFET → cắt nguồn FS1000A
}

// ========== BIT-BANG RF - CÓ POWER CYCLING ==========
void sendRFBitBang(unsigned long code, uint8_t bitLen, uint16_t pulseLen, uint8_t repeat) {
  powerRFOn();
  digitalWrite(RF_SEND_PIN, LOW);
  delayMicroseconds(2000);

  for (uint8_t r = 0; r < repeat; r++) {
    for (int i = bitLen - 1; i >= 0; i--) {
      bool bit = (code >> i) & 0x01;
      if (bit) {
        digitalWrite(RF_SEND_PIN, HIGH);
        delayMicroseconds(pulseLen * 3);
        digitalWrite(RF_SEND_PIN, LOW);
        delayMicroseconds(pulseLen);
      } else {
        digitalWrite(RF_SEND_PIN, HIGH);
        delayMicroseconds(pulseLen);
        digitalWrite(RF_SEND_PIN, LOW);
        delayMicroseconds(pulseLen * 3);
      }
    }
    // Sync
    digitalWrite(RF_SEND_PIN, HIGH);
    delayMicroseconds(pulseLen);
    digitalWrite(RF_SEND_PIN, LOW);
    delayMicroseconds(pulseLen * 31);
  }
  digitalWrite(RF_SEND_PIN, LOW);

  powerRFOff();
}

// ========== RF QUEUE ==========
void enqueueRF(unsigned long code, uint8_t bitLen, uint16_t pulseLen, uint8_t repeat) {
  int next = (rfHead + 1) % RF_QUEUE_SIZE;
  if (next == rfTail) {
    Serial.println("[RF] Queue FULL!");
    return;
  }
  rfQueue[rfHead] = {code, bitLen, pulseLen, repeat};
  rfHead = next;
}

bool dequeueRF(RFCommand &cmd) {
  if (rfHead == rfTail) return false;
  cmd = rfQueue[rfTail];
  rfTail = (rfTail + 1) % RF_QUEUE_SIZE;
  return true;
}
