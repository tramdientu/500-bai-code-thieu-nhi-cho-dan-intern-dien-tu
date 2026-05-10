#include <IRrecv.h>
#include <IRutils.h>

#define IR_RECV_PIN 33

IRrecv irrecv(IR_RECV_PIN, 1024, 50, true);  // Buffer lớn hơn
decode_results results;

void setup() {
  Serial.begin(115200);
  irrecv.enableIRIn();
  irrecv.setTolerance(50);  // Tăng dung sai lên 50%
  Serial.println("Thu IR mở rộng - sẵn sàng...");
}

void loop() {
  if (irrecv.decode(&results)) {

    Serial.println("=== NHẬN ĐƯỢC TÍN HIỆU ===");
    Serial.printf("Protocol: %s\n", typeToString(results.decode_type).c_str());
    Serial.printf("HEX: 0x%llX\n", results.value);
    Serial.printf("Bits: %d\n", results.bits);
    Serial.printf("Raw length: %d\n", results.rawlen);

    // In raw
    Serial.print("Raw: ");
    for (uint16_t i = 0; i < results.rawlen; i++) {
      Serial.printf("%d ", results.rawbuf[i] * kRawTick);
    }
    Serial.println("\n");

    irrecv.resume();
  }
}
