#include "globals.h"
#include "devices.h"
// ===== Object =====
IRsend       irsend(IR_SEND_PIN);
IRFujitsuAC  fujitsuAC(IR_SEND_PIN);
DHT          dht(DHTPIN, DHTTYPE);

// ===== Biến DHT =====
float g_temp = 25.0;

float g_hum  = 50.0;
unsigned long lastDHT = 0;

// ===== RF Queue =====
RFCommand rfQueue[RF_QUEUE_SIZE];
volatile int rfHead = 0;
volatile int rfTail = 0;
