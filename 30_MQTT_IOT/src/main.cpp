#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// =====================
// Cấu hình Pin & Sensor
// =====================
#define DHTPIN  26
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// =====================
// Cấu hình WiFi
// =====================
const char *ssid     = "";
const char *password = "";

// =====================
// Cấu hình MQTT
// =====================
const char *mqtt_server = "";

WiFiClient   espClient;
PubSubClient client(espClient);

// =====================
// Timing
// =====================
unsigned long previousMillis    = 0;
unsigned long mqttRetryMillis   = 0;
const long    SENSOR_INTERVAL   = 5000;   // Đọc sensor mỗi 5 giây
const long    MQTT_RETRY_DELAY  = 3000;   // Retry MQTT mỗi 3 giây

// ============================================================
// Kết nối WiFi (blocking – chỉ gọi 1 lần trong setup)
// ============================================================
void connectWiFi()
{
    Serial.print("Connecting WiFi");
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.println("WiFi Connected");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
}

// ============================================================
// Kết nối MQTT – Non-blocking (dùng millis thay vì while)
// ============================================================
void reconnectMQTT()
{
    if (client.connected()) return;

    unsigned long now = millis();
    if (now - mqttRetryMillis < MQTT_RETRY_DELAY) return; // Chưa đến lúc retry
    mqttRetryMillis = now;

    Serial.print("Connecting MQTT... ");

    if (client.connect("ESP32_DEV_DHT22"))
    {
        Serial.println("OK");
    }
    else
    {
        Serial.print("Failed, state=");
        Serial.println(client.state());
    }
}

// ============================================================
// Hiển thị lỗi lên LCD
// ============================================================
void showLcdError(const char *line1, const char *line2 = "")
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(line1);
    if (strlen(line2) > 0)
    {
        lcd.setCursor(0, 1);
        lcd.print(line2);
    }
}

// ============================================================
// Setup
// ============================================================
void setup()
{
    Serial.begin(115200);

    // Khởi tạo I2C với custom SDA=32, SCL=33
    Wire.begin(32, 33);

    // ✅ Khởi tạo LCD
    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("  ESP32 Ready   ");
    lcd.setCursor(0, 1);
    lcd.print(" Connecting...  ");

    // Khởi tạo DHT
    dht.begin();

    // Kết nối WiFi
    connectWiFi();

    // Cấu hình MQTT server
    client.setServer(mqtt_server, 1883);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi OK!");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP());
    delay(2000);
}

// ============================================================
// Loop
// ============================================================
void loop()
{
    // Giữ kết nối MQTT (non-blocking)
    reconnectMQTT();
    client.loop();

    unsigned long now = millis();
    if (now - previousMillis < SENSOR_INTERVAL) return;
    previousMillis = now;

    // --- Đọc DHT ---
    Serial.println("Reading DHT...");
    float temperature = dht.readTemperature();
    float humidity    = dht.readHumidity();

    if (isnan(temperature) || isnan(humidity))
    {
        Serial.println("[ERROR] DHT read failed!");
        showLcdError("DHT Error!", "Check sensor");
        return;
    }

    // --- Chuyển float → chuỗi ---
    char temp[10];
    char hum[10];
    dtostrf(temperature, 5, 1, temp);
    dtostrf(humidity,    5, 1, hum);

    Serial.printf("Temp: %s C  |  Hum: %s %%\n", temp, hum);

    // --- Publish MQTT ---
    bool ok1 = client.publish("home/esp32dev/temperature", temp);
    bool ok2 = client.publish("home/esp32dev/humidity",    hum);

    // ✅ Log kết quả publish
    if (!ok1) Serial.println("[WARN] Publish temperature FAILED");
    if (!ok2) Serial.println("[WARN] Publish humidity FAILED");

    // --- Hiển thị LCD ---
    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("Temp:");
    lcd.print(temp);
    lcd.write(223); // Ký tự °
    lcd.print("C");

    lcd.setCursor(0, 1);
    lcd.print("Hum :");
    lcd.print(hum);
    lcd.print("%");
}
