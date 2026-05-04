/*
 * ===================================================================
 * Created by Trạm Điện Tử
 * ESP32-S3 Hardware Monitor kết nối wifi
 * Hien thi: CPU Temp, GPU Temp, RAM, Disk, Network Speed
 * ===================================================================
 */

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <SPI.h>

// ============ CAU HINH ============
const char* WIFI_SSID   = "";
const char* WIFI_PASS   = "";
const char* SERVER_IP   = "";
const int   SERVER_PORT = 8085;
const unsigned long FETCH_MS = 2000;

// ============ MAU SAC ============
#define C_BG       0x0000
#define C_HEADER   0x1082
#define C_TEXT     0xFFFF
#define C_LABEL    0x7BEF
#define C_VALUE    0x07FF
#define C_GREEN    0x07E0
#define C_YELLOW   0xFFE0
#define C_ORANGE   0xFD20
#define C_RED      0xF800
#define C_BAR_BG   0x2104
#define C_DIV      0x3186

// ============ OBJECTS ============
TFT_eSPI tft = TFT_eSPI();

// ============ DU LIEU ============
struct HWData {
  float cpuTemp;
  float gpuTemp;
  float ramUsed, ramAvail;
  float ramLoad;
  float ssdUsedPct, ssdFree, ssdTotal;
  float hddUsedPct, hddFree, hddTotal;
  float wifiUpSpeed, wifiDownSpeed;  // bytes/s
  bool valid;
} hw = {};

unsigned long lastFetch = 0;
String serverURL;

// ============ TIEN ICH ============
float pVal(const char* s) {
  return (s && s[0]) ? atof(s) : 0.0f;
}

uint16_t colTemp(float t) {
  if (t < 50) return C_GREEN;
  if (t < 70) return C_YELLOW;
  if (t < 85) return C_ORANGE;
  return C_RED;
}

uint16_t colLoad(float l) {
  if (l < 50) return C_GREEN;
  if (l < 80) return C_YELLOW;
  if (l < 95) return C_ORANGE;
  return C_RED;
}

// Format bytes/s thanh chuoi dep
void formatSpeed(float bytesPerSec, char* buf, int bufSize) {
  if (bytesPerSec >= 1048576)
    snprintf(buf, bufSize, "%.1f MB/s", bytesPerSec / 1048576.0f);
  else if (bytesPerSec >= 1024)
    snprintf(buf, bufSize, "%.1f KB/s", bytesPerSec / 1024.0f);
  else
    snprintf(buf, bufSize, "%.0f B/s", bytesPerSec);
}

// ============ VE BAR ============
void drawBar(int x, int y, int w, int h, float pct, uint16_t col) {
  pct = constrain(pct, 0, 100);
  tft.fillRoundRect(x, y, w, h, 3, C_BAR_BG);
  int fw = (int)(w * pct / 100.0f);
  if (fw > 0) tft.fillRoundRect(x, y, fw, h, 3, col);
}

// ============ PARSE JSON ============
void parseJSON(const String& payload) {
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, payload);
  if (err) { Serial.printf("JSON err: %s\n", err.c_str()); return; }

  JsonArray comps = doc["Children"].as<JsonArray>();
  if (!comps || comps.size() == 0) return;
  JsonArray hws = comps[0]["Children"].as<JsonArray>();
  if (!hws) return;

  for (JsonObject nd : hws) {
    const char* nm = nd["Text"];
    if (!nm) continue;

    // --- CPU Temp ---
    if (strstr(nm, "i7-7700HQ")) {
      for (JsonObject cat : nd["Children"].as<JsonArray>()) {
        const char* cn = cat["Text"];
        if (!cn || strcmp(cn, "Temperatures") != 0) continue;
        for (JsonObject s : cat["Children"].as<JsonArray>()) {
          const char* sn = s["Text"];
          if (sn && !strcmp(sn, "CPU Package"))
            hw.cpuTemp = pVal(s["Value"]);
        }
      }
    }

    // --- GPU Temp ---
    else if (strstr(nm, "Quadro M1200")) {
      for (JsonObject cat : nd["Children"].as<JsonArray>()) {
        const char* cn = cat["Text"];
        if (!cn || strcmp(cn, "Temperatures") != 0) continue;
        for (JsonObject s : cat["Children"].as<JsonArray>()) {
          const char* sn = s["Text"];
          if (sn && !strcmp(sn, "GPU Core"))
            hw.gpuTemp = pVal(s["Value"]);
        }
      }
    }

    // --- RAM ---
    else if (!strcmp(nm, "Total Memory")) {
      for (JsonObject cat : nd["Children"].as<JsonArray>()) {
        const char* cn = cat["Text"];
        if (!cn) continue;
        for (JsonObject s : cat["Children"].as<JsonArray>()) {
          const char* sn = s["Text"];
          const char* sv = s["Value"];
          if (!sn || !sv) continue;
          if (!strcmp(cn, "Load") && !strcmp(sn, "Memory"))
            hw.ramLoad = pVal(sv);
          else if (!strcmp(cn, "Data")) {
            if (!strcmp(sn, "Memory Used")) hw.ramUsed = pVal(sv);
            else if (!strcmp(sn, "Memory Available")) hw.ramAvail = pVal(sv);
          }
        }
      }
    }

    // --- SSD ---
    else if (strstr(nm, "Samsung SSD 980")) {
      for (JsonObject cat : nd["Children"].as<JsonArray>()) {
        const char* cn = cat["Text"];
        if (!cn) continue;
        for (JsonObject s : cat["Children"].as<JsonArray>()) {
          const char* sn = s["Text"];
          const char* sv = s["Value"];
          if (!sn || !sv) continue;
          if (!strcmp(cn, "Load") && !strcmp(sn, "Used Space"))
            hw.ssdUsedPct = pVal(sv);
          else if (!strcmp(cn, "Data")) {
            if (!strcmp(sn, "Free Space")) hw.ssdFree = pVal(sv);
            else if (!strcmp(sn, "Total Space")) hw.ssdTotal = pVal(sv);
          }
        }
      }
    }

    // --- HDD ---
    else if (strstr(nm, "ST1000LM035")) {
      for (JsonObject cat : nd["Children"].as<JsonArray>()) {
        const char* cn = cat["Text"];
        if (!cn) continue;
        for (JsonObject s : cat["Children"].as<JsonArray>()) {
          const char* sn = s["Text"];
          const char* sv = s["Value"];
          if (!sn || !sv) continue;
          if (!strcmp(cn, "Load") && !strcmp(sn, "Used Space"))
            hw.hddUsedPct = pVal(sv);
          else if (!strcmp(cn, "Data")) {
            if (!strcmp(sn, "Free Space")) hw.hddFree = pVal(sv);
            else if (!strcmp(sn, "Total Space")) hw.hddTotal = pVal(sv);
          }
        }
      }
    }

    // --- WiFi Speed ---
    else if (!strcmp(nm, "Wi-Fi")) {
      for (JsonObject cat : nd["Children"].as<JsonArray>()) {
        const char* cn = cat["Text"];
        if (!cn || strcmp(cn, "Throughput") != 0) continue;
        for (JsonObject s : cat["Children"].as<JsonArray>()) {
          const char* sn = s["Text"];
          const char* rv = s["RawValue"];
          if (!sn || !rv) continue;
          if (!strcmp(sn, "Upload Speed"))
            hw.wifiUpSpeed = pVal(rv);
          else if (!strcmp(sn, "Download Speed"))
            hw.wifiDownSpeed = pVal(rv);
        }
      }
    }
  }
  hw.valid = true;
}

// ============ FETCH DATA ============
void fetchData() {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.begin("http://192.168.1.141:8085/data.json");
  http.setTimeout(3000);
  int code = http.GET();

  if (code == HTTP_CODE_OK) {
    String payload = http.getString();
    parseJSON(payload);
  } else {
    Serial.printf("HTTP err: %d\n", code);
  }
  http.end();
}

// ============ VE MAN HINH ============
void drawScreen() {
  tft.fillScreen(C_BG);
  char b[48];
  int y = 0;

  // === HEADER ===
  tft.fillRect(0, 0, 320, 26, C_HEADER);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(C_VALUE, C_HEADER);
  tft.drawString("HW MONITOR", 160, 13, 2);

  // WiFi dot
  uint16_t wc = (WiFi.status() == WL_CONNECTED) ? C_GREEN : C_RED;
  tft.fillCircle(302, 13, 4, wc);

  if (!hw.valid) {
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(C_LABEL, C_BG);
    tft.drawString("Dang ket noi...", 160, 130, 2);
    return;
  }

  y = 32;

  // === 🌡️ NHIET DO ===
  // CPU Temp
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(C_LABEL, C_BG);
  tft.drawString("CPU Temp", 10, y, 2);
  snprintf(b, 48, "%.0f C", hw.cpuTemp);
  tft.setTextDatum(MR_DATUM);
  tft.setTextColor(colTemp(hw.cpuTemp), C_BG);
  tft.drawString(b, 155, y, 2);

  // GPU Temp
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(C_LABEL, C_BG);
  tft.drawString("GPU Temp", 170, y, 2);
  snprintf(b, 48, "%.0f C", hw.gpuTemp);
  tft.setTextDatum(MR_DATUM);
  tft.setTextColor(colTemp(hw.gpuTemp), C_BG);
  tft.drawString(b, 310, y, 2);

  y += 22;
  tft.drawLine(5, y, 315, y, C_DIV);
  y += 6;

  // === 💾 RAM ===
  float ramTotal = hw.ramUsed + hw.ramAvail;
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(C_TEXT, C_BG);
  tft.drawString("RAM", 10, y, 2);
  snprintf(b, 48, "%.1f / %.1f GB  (%.0f%%)", hw.ramUsed, ramTotal, hw.ramLoad);
  tft.setTextDatum(MR_DATUM);
  tft.setTextColor(colLoad(hw.ramLoad), C_BG);
  tft.drawString(b, 310, y, 2);
  y += 18;
  drawBar(10, y, 300, 12, hw.ramLoad, colLoad(hw.ramLoad));

  y += 20;
  tft.drawLine(5, y, 315, y, C_DIV);
  y += 6;

  // === 💿 SSD ===
  float ssdUsedGB = hw.ssdTotal - hw.ssdFree;
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(C_TEXT, C_BG);
  tft.drawString("SSD 980", 10, y, 2);
  snprintf(b, 48, "%.0f / %.0f GB  (%.0f%%)", ssdUsedGB, hw.ssdTotal, hw.ssdUsedPct);
  tft.setTextDatum(MR_DATUM);
  tft.setTextColor(colLoad(hw.ssdUsedPct), C_BG);
  tft.drawString(b, 310, y, 2);
  y += 18;
  drawBar(10, y, 300, 12, hw.ssdUsedPct, colLoad(hw.ssdUsedPct));

  y += 20;

  // === 💿 HDD ===
  float hddUsedGB = hw.hddTotal - hw.hddFree;
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(C_TEXT, C_BG);
  tft.drawString("HDD 1TB", 10, y, 2);
  snprintf(b, 48, "%.0f / %.0f GB  (%.0f%%)", hddUsedGB, hw.hddTotal, hw.hddUsedPct);
  tft.setTextDatum(MR_DATUM);
  tft.setTextColor(colLoad(hw.hddUsedPct), C_BG);
  tft.drawString(b, 310, y, 2);
  y += 18;
  drawBar(10, y, 300, 12, hw.hddUsedPct, colLoad(hw.hddUsedPct));

  y += 20;
  tft.drawLine(5, y, 315, y, C_DIV);
  y += 6;

  // === 🌐 NETWORK ===
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(C_TEXT, C_BG);
  tft.drawString("Network (WiFi)", 10, y, 2);
  y += 20;

  // Upload
  char speedBuf[24];
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(C_LABEL, C_BG);
  tft.drawString("UP:", 10, y, 2);
  formatSpeed(hw.wifiUpSpeed, speedBuf, 24);
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(C_GREEN, C_BG);
  tft.drawString(speedBuf, 50, y, 2);

  // Download
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(C_LABEL, C_BG);
  tft.drawString("DOWN:", 170, y, 2);
  formatSpeed(hw.wifiDownSpeed, speedBuf, 24);
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(C_VALUE, C_BG);
  tft.drawString(speedBuf, 225, y, 2);
}

// ============ SETUP ============
void setup() {
  Serial.begin(115200);
  Serial.println("\n=== HW Monitor Simple ===");

  // TFT
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(C_BG);
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  // Splash
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(C_VALUE, C_BG);
  tft.drawString("HW Monitor", 160, 100, 4);
  tft.setTextColor(C_LABEL, C_BG);
  tft.drawString("Connecting WiFi...", 160, 140, 2);

  // WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 30) {
    delay(500);
    Serial.print(".");
    retry++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\nWiFi OK! IP: %s\n", WiFi.localIP().toString().c_str());
    tft.setTextColor(C_GREEN, C_BG);
    tft.drawString("WiFi Connected!", 160, 170, 2);
  } else {
    Serial.println("\nWiFi FAILED!");
    tft.setTextColor(C_RED, C_BG);
    tft.drawString("WiFi Failed!", 160, 170, 2);
  }

  serverURL = "http://" + String(SERVER_IP) + ":" + String(SERVER_PORT) + "/data.json";
  delay(1000);
}

// ============ LOOP ============
void loop() {
  // Fetch du lieu dinh ky
  if (millis() - lastFetch >= FETCH_MS) {
    lastFetch = millis();
    fetchData();
    drawScreen();
  }

  // Reconnect WiFi neu mat
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi lost, reconnecting...");
    WiFi.reconnect();
    delay(2000);
  }
}
