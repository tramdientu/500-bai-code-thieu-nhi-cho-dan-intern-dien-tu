#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <TFT_eSPI.h>
#include <SPI.h>

// ============ CAU HINH ============
const char* WIFI_SSID   = "";
const char* WIFI_PASS   = "";
const char* MQTT_SERVER = "";

// ============ PURE BLACK COLOR PALETTE ============
#define C_BLACK        0x0000
#define C_WHITE        0xFFFF
#define C_GRAY         0x7BEF

// Nhiet do (Tong nong)
#define C_TEMP_COLD    0x07FF  // Cyan   < 25°C
#define C_TEMP_OK      0x07E0  // Green  25~35°C
#define C_TEMP_WARM    0xFFE0  // Yellow 35~40°C
#define C_TEMP_HOT     0xFD20  // Orange 40~45°C
#define C_TEMP_FIRE    0xF800  // Red    > 45°C

// Do am (Tong lanh)
#define C_HUM_DRY      0xFDB0  // Cam nhat  < 30%
#define C_HUM_LOW      0xFFE0  // Vang      30~50%
#define C_HUM_OK       0x07E0  // Xanh la   50~70%
#define C_HUM_HIGH     0x07FF  // Cyan      70~85%
#define C_HUM_WET      0x001F  // Blue      > 85%

// ============ OBJECTS ============
TFT_eSPI tft = TFT_eSPI();
WiFiClient espClient;
PubSubClient client(espClient);

// ============ DU LIEU ============
String temp = "--";
String hum  = "--";

// ============ LOGIC MAU SAC ============
uint16_t colTemp(float t) {
    if (t < 25) return C_TEMP_COLD;
    if (t < 35) return C_TEMP_OK;
    if (t < 40) return C_TEMP_WARM;
    if (t < 45) return C_TEMP_HOT;
    return C_TEMP_FIRE;
}

uint16_t colHum(float h) {
    if (h < 30) return C_HUM_DRY;
    if (h < 50) return C_HUM_LOW;
    if (h < 70) return C_HUM_OK;
    if (h < 85) return C_HUM_HIGH;
    return C_HUM_WET;
}

// ============ VE BAR (Minimalist) ============
void drawBar(int x, int y, int w, int h, float pct, uint16_t col) {
    pct = constrain(pct, 0, 100);
    // Xoa bar cu
    tft.fillRect(x, y, w, h, C_BLACK);
    // Ve duong nen
    tft.drawFastHLine(x, y + h / 2, w, 0x2104);
    // Ve phan tien trinh
    int fw = (int)(w * pct / 100.0f);
    if (fw > 0) tft.fillRect(x, y, fw, h, col);
}

// ============ VE UI CO DINH (1 LAN) ============
void drawUI() {
    tft.fillScreen(C_BLACK);

    // Header
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(C_WHITE, C_BLACK);
    tft.drawString("ENVIRONMENT MONITOR", 160, 16, 2);
    tft.drawFastHLine(20, 28, 280, C_GRAY);

    // Label nhiet do
    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(C_GRAY, C_BLACK);
    tft.drawString("TEMP", 20, 55, 2);

    // Label do am
    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(C_GRAY, C_BLACK);
    tft.drawString("HUMIDITY", 20, 148, 2);

    // Duong ke phan cach
    tft.drawFastHLine(20, 138, 280, 0x2104);
    tft.drawFastHLine(20, 210, 280, 0x2104);

    // Footer label
    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(C_GRAY, C_BLACK);
    tft.drawString("STATUS:", 20, 224, 1);
}

// ============ UPDATE TEMP ============
void updateTemp() {
    float val = temp.toFloat();
    uint16_t col = (temp == "--") ? C_GRAY : colTemp(val);

    // Xoa vung so cu
    tft.fillRect(80, 48, 230, 40, C_BLACK);

    // Ve so moi
    char buf[20];
    if (temp != "--") snprintf(buf, 20, "%.1f C", val);
    else strcpy(buf, "-- C");

    tft.setTextDatum(MR_DATUM);
    tft.setTextColor(col, C_BLACK);
    tft.drawString(buf, 308, 68, 4);

    // Update bar
    float pct = (temp != "--") ? constrain(val / 50.0f * 100.0f, 0, 100) : 0;
    drawBar(20, 100, 280, 8, pct, col);
}

// ============ UPDATE HUM ============
void updateHum() {
    float val = hum.toFloat();
    uint16_t col = (hum == "--") ? C_GRAY : colHum(val);

    // Xoa vung so cu
    tft.fillRect(80, 142, 230, 40, C_BLACK);

    // Ve so moi
    char buf[20];
    if (hum != "--") snprintf(buf, 20, "%.1f %%", val);
    else strcpy(buf, "-- %%");

    tft.setTextDatum(MR_DATUM);
    tft.setTextColor(col, C_BLACK);
    tft.drawString(buf, 308, 162, 4);

    // Update bar
    float pct = (hum != "--") ? val : 0;
    drawBar(20, 192, 280, 8, pct, col);
}

// ============ UPDATE WIFI DOT ============
void updateWifiDot() {
    uint16_t wc = (WiFi.status() == WL_CONNECTED) ? C_TEMP_OK : C_TEMP_FIRE;
    tft.fillCircle(305, 16, 4, wc);
    tft.drawCircle(305, 16, 5, C_GRAY);
}

// ============ UPDATE MQTT STATUS (FIXED) ============
void updateMqttStatus() {
    // Xoa vung du rong de khong bi de len nhau
    tft.fillRect(70, 218, 240, 16, C_BLACK);

    tft.setTextDatum(ML_DATUM);
    if (client.connected()) {
        tft.setTextColor(C_TEMP_OK, C_BLACK);
        tft.drawString("CONNECTED", 72, 224, 1);
    } else {
        tft.setTextColor(C_TEMP_FIRE, C_BLACK);
        tft.drawString("DISCONNECTED", 72, 224, 1);
    }
}

// ============ MQTT CALLBACK ============
void callback(char* topic, byte* payload, unsigned int length) {
    String message;
    for (unsigned int i = 0; i < length; i++) message += (char)payload[i];

    Serial.printf("[MQTT] %s -> %s\n", topic, message.c_str());

    if (String(topic) == "home/esp32dev/temperature") {
        temp = message;
        updateTemp();
    }
    if (String(topic) == "home/esp32dev/humidity") {
        hum = message;
        updateHum();
    }
}

// ============ WIFI ============
void connectWiFi() {
    Serial.print("Connecting WiFi");
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    int retry = 0;
    while (WiFi.status() != WL_CONNECTED && retry < 30) {
        delay(500);
        Serial.print(".");
        retry++;
    }
    Serial.println(WiFi.status() == WL_CONNECTED ? "\nWiFi OK" : "\nWiFi FAIL");
}

// ============ MQTT RECONNECT ============
void reconnectMQTT() {
    while (!client.connected()) {
        Serial.print("MQTT connecting...");
        if (client.connect("ESP32_S3_BLACK")) {
            Serial.println("OK");
            client.subscribe("home/esp32dev/temperature");
            client.subscribe("home/esp32dev/humidity");
        } else {
            Serial.printf("Failed: %d\n", client.state());
            delay(3000);
        }
    }
}

// ============ SETUP ============
void setup() {
    Serial.begin(115200);

    tft.init();
    tft.setRotation(1);
    tft.fillScreen(C_BLACK);
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);

    // Splash screen
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(C_WHITE, C_BLACK);
    tft.drawString("ENVIRONMENT MONITOR", 160, 100, 2);
    tft.setTextColor(C_GRAY, C_BLACK);
    tft.drawString("Connecting WiFi...", 160, 130, 2);

    connectWiFi();

    if (WiFi.status() == WL_CONNECTED) {
        tft.setTextColor(C_TEMP_OK, C_BLACK);
        tft.drawString("WiFi Connected!", 160, 155, 2);
    } else {
        tft.setTextColor(C_TEMP_FIRE, C_BLACK);
        tft.drawString("WiFi Failed!", 160, 155, 2);
    }

    delay(800);

    // Ve UI chinh
    drawUI();
    updateTemp();
    updateHum();
    updateWifiDot();
    updateMqttStatus();

    client.setServer(MQTT_SERVER, 1883);
    client.setCallback(callback);
}

// ============ LOOP ============
void loop() {
    if (WiFi.status() != WL_CONNECTED) {
        WiFi.reconnect();
        delay(2000);
        updateWifiDot();
    }

    if (!client.connected()) {
        reconnectMQTT();
        updateMqttStatus();
    }

    client.loop();
}
