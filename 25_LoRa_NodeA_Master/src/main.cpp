#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <DHT.h>

#include <MD_Parola.h>
#include <MD_MAX72xx.h>

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES   4
#define MATRIX_CS     15

MD_Parola matrix(
    HARDWARE_TYPE,
    MATRIX_CS,
    MAX_DEVICES
);

// =========================
// DHT22
// =========================

#define DHTPIN   4
#define DHTTYPE  DHT22

DHT dht(DHTPIN, DHTTYPE);

// =========================
// LoRa
// =========================

#define LORA_SS    5
#define LORA_RST   14
#define LORA_DIO0  26

#define LORA_SCK   18
#define LORA_MISO  19
#define LORA_MOSI  23

// =========================
unsigned long lastDisplay = 0;
bool showTemp = true;

float currentTemp = 0;
float currentHum = 0;

unsigned long lastSend = 0;

String displayText = "BOOTING...";

void setup()
{
    Serial.begin(115200);

    // SPI dùng chung cho LoRa + Matrix
    SPI.begin(
        LORA_SCK,
        LORA_MISO,
        LORA_MOSI
    );

    // =====================
    // Matrix
    // =====================

    matrix.begin();

    matrix.setIntensity(2);

    matrix.displayClear();

    matrix.displayText(
        "BOOT...",
        PA_CENTER,
        50,
        1000,
        PA_SCROLL_LEFT,
        PA_SCROLL_LEFT
    );

    // =====================
    // DHT22
    // =====================

    dht.begin();

    // =====================
    // LoRa
    // =====================

    LoRa.setPins(
        LORA_SS,
        LORA_RST,
        LORA_DIO0
    );

    while (!LoRa.begin(433E6))
    {
        Serial.println("LoRa init failed");

        matrix.displayClear();

        matrix.displayText(
            "LORA FAIL",
            PA_LEFT,
            50,
            0,
            PA_SCROLL_LEFT,
            PA_SCROLL_LEFT
        );

        delay(1000);
    }

    LoRa.setTxPower(20);

    Serial.println("LoRa Ready");

    displayText = "LORA READY";
}

void loop()
{
    matrix.displayAnimate();

    if (millis() - lastSend >= 10000)
    {
        lastSend = millis();

        float temp = dht.readTemperature();
        float hum  = dht.readHumidity();

        if (isnan(temp) || isnan(hum))
        {
            Serial.println("DHT22 ERROR");

            displayText = "DHT22 ERROR";

            matrix.displayClear();

            matrix.displayText(
                displayText.c_str(),
                PA_LEFT,
                50,
                0,
                PA_SCROLL_LEFT,
                PA_SCROLL_LEFT
            );

            return;
        }

        // Payload gửi LoRa
        String payload =
            String(temp, 1) +
            "," +
            String(hum, 1);

        LoRa.beginPacket();
        LoRa.print(payload);
        LoRa.endPacket();

        Serial.print("Sent -> ");
        Serial.println(payload);

        // Nội dung hiển thị Matrix
        displayText =
            "TEMP "
            + String(temp, 1)
            + "C  HUM "
            + String(hum, 1)
            + "%";

        matrix.displayClear();

        matrix.displayText(
            displayText.c_str(),
            PA_LEFT,
            50,
            0,
            PA_SCROLL_LEFT,
            PA_SCROLL_LEFT
        );
    }
}