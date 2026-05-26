#include "devices.h"

// ========== TempSensor ==========
TempSensor::TempSensor() : Service::TemperatureSensor() {
  temp = new Characteristic::CurrentTemperature(0);
  temp->setRange(-40, 80);
  lastVal = -999;
  lastUpdate = 0;
}

void TempSensor::loop() {
  if (millis() - lastUpdate < 5000) return;
  lastUpdate = millis();
  if (fabs(g_temp - lastVal) >= 0.1) {
    temp->setVal(g_temp);
    lastVal = g_temp;
  }
}

// ========== HumSensor ==========
HumSensor::HumSensor() : Service::HumiditySensor() {
  hum = new Characteristic::CurrentRelativeHumidity(0);
  lastVal = -999;
  lastUpdate = 0;
}

void HumSensor::loop() {
  if (millis() - lastUpdate < 5000) return;
  lastUpdate = millis();
  if (fabs(g_hum - lastVal) >= 1.0) {
    hum->setVal(g_hum);
    lastVal = g_hum;
  }
}
