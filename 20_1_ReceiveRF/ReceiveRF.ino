/*
 * ESP32 + RF433 Receiver
 * Hiển thị mã code dễ copy để dùng lại
 */

#include <RCSwitch.h>

RCSwitch mySwitch = RCSwitch();

#define RF_RECEIVE_PIN 17   // Đổi nếu bạn dùng chân khác

void setup() {
  Serial.begin(115200);
  // ESP32 KHÔNG dùng interrupt number như Arduino Uno
  mySwitch.enableReceive(digitalPinToInterrupt(RF_RECEIVE_PIN));
  
  Serial.println();
  Serial.println("✅ ESP32 san sang nhan tin hieu RF 433MHz...");
  Serial.println("==============================================");
}

void loop() {

  if (mySwitch.available()) {
    
    unsigned long receivedCode = mySwitch.getReceivedValue();
    unsigned int bitLength = mySwitch.getReceivedBitlength();
    unsigned int protocol = mySwitch.getReceivedProtocol();
    
    Serial.println();
    Serial.println("===== 📡 TIN HIEU NHAN DUOC =====");
    
    // ✅ Format để copy sang code phát
    Serial.print("rf_code_to_send = ");
    Serial.print(receivedCode);
    Serial.println(";");
    
    Serial.print("bit_length = ");
    Serial.print(bitLength);
    Serial.println(";");
    
    Serial.print("protocol = ");
    Serial.print(protocol);
    Serial.println(";");
    
    Serial.println("=================================");
    
    // In thêm dạng nhị phân cho dễ nhìn
    Serial.print("Binary: ");
    Serial.println(receivedCode, BIN);

    mySwitch.resetAvailable();
  }
}
