#include "devices.h"

void readDHT() {
  if (millis() - lastDHT < 5000) return;
  lastDHT = millis();

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (!isnan(t) && !isnan(h)) {
    g_temp = t + TEMP_OFFSET;
    g_hum  = h + HUM_OFFSET;
    if (g_hum < 0)   g_hum = 0;
    if (g_hum > 100) g_hum = 100;
    Serial.printf("[DHT] T: %.1f | H: %.1f\n", g_temp, g_hum);
  } else {
    Serial.println("[DHT] FAIL");
  }
}
