#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define SDA_PIN 4
#define SCL_PIN 5

LiquidCrystal_I2C lcd(0x27, 16, 2);

#define LORA_SS    10
#define LORA_RST   9
#define LORA_DIO0  8

#define LORA_SCK   12
#define LORA_MISO  13
#define LORA_MOSI  11

void setup()
{
    Serial.begin(115200);

    Wire.begin(SDA_PIN, SCL_PIN);

    lcd.init();
    lcd.backlight();

    lcd.setCursor(0,0);
    lcd.print("LoRa Receiver");

    SPI.begin(
        LORA_SCK,
        LORA_MISO,
        LORA_MOSI,
        LORA_SS
    );

    LoRa.setPins(
        LORA_SS,
        LORA_RST,
        LORA_DIO0
    );

    while (!LoRa.begin(433E6))
    {
        Serial.println("LoRa init failed");

        lcd.clear();
        lcd.print("LoRa Failed");

        delay(1000);
    }

    lcd.clear();
    lcd.print("LoRa Ready");
}

void loop()
{
    int packetSize = LoRa.parsePacket();

    if(packetSize)
    {
        String msg;

        while(LoRa.available())
        {
            msg += (char)LoRa.read();
        }

        int comma = msg.indexOf(',');

        if(comma > 0)
        {
            float temp =
                msg.substring(0, comma).toFloat();

            float hum =
                msg.substring(comma + 1).toFloat();

            Serial.print("Temp: ");
            Serial.println(temp);

            Serial.print("Hum : ");
            Serial.println(hum);

            int rssi = LoRa.packetRssi();
            float snr = LoRa.packetSnr();

            lcd.clear();

            // Dòng 1
            lcd.setCursor(0, 0);
            lcd.print("T:");
            lcd.print(temp, 1);
            lcd.print("C ");

            lcd.print("H:");
            lcd.print((int)hum);
            lcd.print("%");

            // Dòng 2
            lcd.setCursor(0, 1);
            lcd.print("R:");
            lcd.print(rssi);

            lcd.print(" S:");
            lcd.print(snr, 1);
        }
    }
}