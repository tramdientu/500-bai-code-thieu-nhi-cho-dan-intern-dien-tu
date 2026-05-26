#include "devices.h"
void sendIR(uint32_t code) {
  irsend.sendNEC(code, 32);
  Serial.printf("[IR] 0x%08X\n", code);
}
