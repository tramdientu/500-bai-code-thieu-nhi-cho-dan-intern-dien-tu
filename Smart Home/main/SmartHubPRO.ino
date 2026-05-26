#include <HomeSpan.h>
#include "devices.h"    

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(RF_POWER_PIN, OUTPUT);
  digitalWrite(RF_POWER_PIN, LOW);

  pinMode(RF_SEND_PIN, OUTPUT);
  digitalWrite(RF_SEND_PIN, LOW);

  irsend.begin();
  dht.begin();

  homeSpan.setPairingCode(PAIRING_CODE);
  homeSpan.setWifiCredentials(WIFI_SSID, WIFI_PASSWORD);
  homeSpan.begin(Category::Bridges, "Smart Hub PRO");

  // ===== Bridge =====
  new SpanAccessory();
    addAccessoryInfo("Smart Hub PRO");
    new Service::HAPProtocolInformation();
      new Characteristic::Version("1.1.0");

  // ===== IR: Đèn LED 1 =====
  new SpanAccessory();
    addAccessoryInfo("Den LED 1");
    new DenLED1();

  // ===== Điều Hòa Fujitsu =====
  new SpanAccessory();
    addAccessoryInfo("Dieu Hoa");
    new DieuHoaFujitsu();

  // ===== RF Devices =====
  new SpanAccessory();
    addAccessoryInfo("Den LED 2");
    new RFDevice("Den LED 2", RF_LED2_ON, RF_LED2_OFF, 24, PULSE_LED2);

  new SpanAccessory();
    addAccessoryInfo("Den LED 3");
    new RFDevice("Den LED 3", RF_LED3_ON, RF_LED3_OFF, 24, PULSE_LED3);

  new SpanAccessory();
    addAccessoryInfo("Den Ban");
    new RFDevice("Den Ban", RF_DESK_ON, RF_DESK_OFF, 24, PULSE_DESK);

  new SpanAccessory();
    addAccessoryInfo("Den Ke Sach");
    new RFDevice("Den Ke Sach", RF_SHELF_ON, RF_SHELF_OFF, 24, PULSE_SHELF);

  // ===== Sensors =====
  new SpanAccessory();
    addAccessoryInfo("Cam Bien Nhiet Do");
    new TempSensor();

  new SpanAccessory();
    addAccessoryInfo("Cam Bien Do Am");
    new HumSensor();

  Serial.println("\n=== SMART HUB PRO READY (v3.0 - MOSFET Power Control) ===");
  Serial.printf("  RF Send Pin    : GPIO %d\n", RF_SEND_PIN);
  Serial.printf("  RF Power Pin   : GPIO %d (MOSFET)\n", RF_POWER_PIN);
  Serial.printf("  Power ON delay : %dms\n", RF_POWER_ON_DELAY_MS);
  Serial.printf("  Power OFF delay: %dms\n", RF_POWER_OFF_DELAY_MS);
  Serial.printf("  LED 2 : %uµs\n", PULSE_LED2);
  Serial.printf("  LED 3 : %uµs\n", PULSE_LED3);
  Serial.printf("  Ban   : %uµs\n", PULSE_DESK);
  Serial.printf("  KeSach: %uµs\n", PULSE_SHELF);
  Serial.println("=====================================================\n");
}

void loop() {
  homeSpan.poll();
  readDHT();

  static unsigned long lastRF = 0;
  if (millis() - lastRF > RF_GAP_MS) {
    RFCommand cmd;
    if (dequeueRF(cmd)) {
      sendRFBitBang(cmd.code, cmd.bitLen, cmd.pulseLen, cmd.repeat);
      Serial.printf("[RF] Sent: %lu | %ub | %uµs | x%u (Power CYCLED)\n",
                    cmd.code, cmd.bitLen, cmd.pulseLen, cmd.repeat);
      lastRF = millis();
    }
  }
}
