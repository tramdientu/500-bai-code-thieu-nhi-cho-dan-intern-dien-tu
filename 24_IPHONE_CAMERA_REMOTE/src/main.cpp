#include <Arduino.h>
#include <BleKeyboard.h>

// ============================================
// Trạm điện tử
// 📷 ESP32-C3 Super Mini - iPhone Camera Remote
// 🎯 Phiên bản hoàn chỉnh: BLE + Buzzer + Serial
// ============================================

BleKeyboard bleKeyboard("iPhone Camera Remote", "Trạm điện tử", 100);

// 🔘 Khai báo chân GPIO (pinout chuẩn cho C3 Super Mini)
#define BTN_CAPTURE   0     // 📸 Chụp ảnh
#define BTN_VIDEO     1     // 🎥 Quay video
#define BTN_BURST     3     // 📸 Burst (chụp liên tiếp)
#define BTN_TIMER     4     // ⏲️ Timer (hẹn giờ 3s)
#define BUZZER_PIN    10    // 🔊 Buzzer

// 🎵 PWM cho buzzer (API CŨ - Core 2.x)
#define BUZZER_CHANNEL  0
#define BUZZER_RES      8

// 🎼 Nốt nhạc (Hz)
#define NOTE_C4   262
#define NOTE_E4   330
#define NOTE_G4   392
#define NOTE_A4   440
#define NOTE_C5   523
#define NOTE_E5   659
#define NOTE_G5   784
#define NOTE_C6   1047

// ⏱️ Thông số thời gian
#define DEBOUNCE_DELAY    50      // ms - chống dội phím
#define BURST_INTERVAL    300     // ms - khoảng giữa 2 lần chụp burst
#define SEARCHING_BEEP    3000    // ms - beep tìm kết nối mỗi 3s

// 🚦 Biến trạng thái
bool isVideoRecording = false;
bool wasConnected = false;
unsigned long lastBurstTime = 0;
unsigned long lastSearchBeep = 0;

bool lastStateCapture = HIGH;
bool lastStateVideo   = HIGH;
bool lastStateTimer   = HIGH;

// Forward declaration
void playTone(int freq, int duration);
void playBootSound();
void playConnectedSound();
void playDisconnectedSound();
void playSearchingBeep();
void playShutterSound();
void playRecordStartSound();
void playRecordStopSound();
void playBurstClick();
void playTickSound();

void checkConnectionStatus();
void handleCaptureButton();
void handleVideoButton();
void handleBurstButton();
void handleTimerButton();

// ============================================
// 🔧 SETUP
// ============================================
void setup() {
  Serial.begin(115200);
  delay(3000);  // ⚠️ Đợi USB CDC khởi động (bắt buộc với C3)
  
  Serial.println("\n========================================");
  Serial.println("📷 iPhone Camera Remote");
  Serial.println("🚀 ESP32-C3 Super Mini");
  Serial.println("========================================");
  Serial.println("📌 Pinout:");
  Serial.printf("   📸 Capture: GPIO %d\n", BTN_CAPTURE);
  Serial.printf("   🎥 Video:   GPIO %d\n", BTN_VIDEO);
  Serial.printf("   📸 Burst:   GPIO %d\n", BTN_BURST);
  Serial.printf("   ⏲️ Timer:   GPIO %d\n", BTN_TIMER);
  Serial.printf("   🔊 Buzzer:  GPIO %d\n", BUZZER_PIN);
  Serial.println("========================================");
  
  // 🔘 Cấu hình chân nút
  pinMode(BTN_CAPTURE, INPUT_PULLUP);
  pinMode(BTN_VIDEO,   INPUT_PULLUP);
  pinMode(BTN_BURST,   INPUT_PULLUP);
  pinMode(BTN_TIMER,   INPUT_PULLUP);
  
  // 🔊 Cấu hình buzzer
  ledcSetup(BUZZER_CHANNEL, 1000, BUZZER_RES);
  ledcAttachPin(BUZZER_PIN, BUZZER_CHANNEL);
  
  // 🎵 Âm thanh khởi động
  Serial.println("🎵 Boot sound...");
  playBootSound();
  
  // 📡 Khởi động BLE
  Serial.println("📡 Khởi động BLE...");
  bleKeyboard.begin();
  
  Serial.println("\n✅ Sẵn sàng!");
  Serial.println("📱 Ghép đôi iPhone với 'iPhone Camera Remote'");
  Serial.println("⏳ Đang chờ kết nối...\n");
}

// ============================================
// 🔄 LOOP CHÍNH
// ============================================
void loop() {
  checkConnectionStatus();
  
  // Xử lý các nút
  handleCaptureButton();
  handleVideoButton();
  handleBurstButton();
  handleTimerButton();
  
  delay(20);
}

// ============================================
// 📡 KIỂM TRA KẾT NỐI BLE
// ============================================
void checkConnectionStatus() {
  bool nowConnected = bleKeyboard.isConnected();
  
  // 🎉 Vừa kết nối thành công
  if (nowConnected && !wasConnected) {
    Serial.println("✅ ĐÃ KẾT NỐI iPhone!");
    playConnectedSound();
    wasConnected = true;
  }
  
  // 💔 Vừa mất kết nối
  if (!nowConnected && wasConnected) {
    Serial.println("❌ MẤT KẾT NỐI!");
    playDisconnectedSound();
    wasConnected = false;
  }
  
  // 🔍 Đang tìm kết nối → beep nhẹ mỗi 3s
  if (!nowConnected) {
    if (millis() - lastSearchBeep > SEARCHING_BEEP) {
      playSearchingBeep();
      lastSearchBeep = millis();
    }
  }
}

// ============================================
// 📸 NÚT CHỤP ẢNH
// ============================================
void handleCaptureButton() {
  bool currentState = digitalRead(BTN_CAPTURE);
  
  if (currentState == LOW && lastStateCapture == HIGH) {
    delay(DEBOUNCE_DELAY);
    if (digitalRead(BTN_CAPTURE) == LOW) {
      Serial.println(">>> 📸 CAPTURE pressed!");
      playShutterSound();
      
      if (bleKeyboard.isConnected()) {
        bleKeyboard.write(KEY_MEDIA_VOLUME_UP);
        Serial.println("    ✅ Đã gửi VOLUME_UP → iPhone chụp ảnh");
      } else {
        Serial.println("    ⚠️ Chưa kết nối BLE");
      }
    }
  }
  lastStateCapture = currentState;
}

// ============================================
// 🎥 NÚT QUAY VIDEO (Toggle)
// ============================================
void handleVideoButton() {
  bool currentState = digitalRead(BTN_VIDEO);
  
  if (currentState == LOW && lastStateVideo == HIGH) {
    delay(DEBOUNCE_DELAY);
    if (digitalRead(BTN_VIDEO) == LOW) {
      Serial.println(">>> 🎥 VIDEO pressed!");
      isVideoRecording = !isVideoRecording;
      
      if (isVideoRecording) {
        Serial.println("    ▶️ Bắt đầu quay video");
        playRecordStartSound();
        
        if (bleKeyboard.isConnected()) {
          // Giữ Volume Up ~1.5s để chuyển sang chế độ video & bắt đầu quay
          bleKeyboard.press(KEY_MEDIA_VOLUME_UP);
          delay(1500);
          bleKeyboard.releaseAll();
          Serial.println("    ✅ Đã gửi lệnh quay");
        } else {
          Serial.println("    ⚠️ Chưa kết nối BLE");
        }
      } else {
        Serial.println("    ⏹️ Dừng quay video");
        playRecordStopSound();
        
        if (bleKeyboard.isConnected()) {
          bleKeyboard.write(KEY_MEDIA_VOLUME_UP);
          Serial.println("    ✅ Đã gửi lệnh dừng");
        }4
      }
    }
  }
  lastStateVideo = currentState;
}

// ============================================
// 📸📸 NÚT BURST (Chụp liên tiếp khi giữ)
// ============================================
void handleBurstButton() {
  if (digitalRead(BTN_BURST) == LOW) {
    if (millis() - lastBurstTime > BURST_INTERVAL) {
      Serial.println(">>> 📸 BURST pressed!");
      playBurstClick();
      
      if (bleKeyboard.isConnected()) {
        bleKeyboard.write(KEY_MEDIA_VOLUME_UP);
        Serial.println("    ✅ Burst shot");
      } else {
        Serial.println("    ⚠️ Chưa kết nối BLE");
      }
      lastBurstTime = millis();
    }
  }
}

// ============================================
// ⏲️ NÚT TIMER (Hẹn giờ 3 giây)
// ============================================
void handleTimerButton() {
  bool currentState = digitalRead(BTN_TIMER);
  
  if (currentState == LOW && lastStateTimer == HIGH) {
    delay(DEBOUNCE_DELAY);
    if (digitalRead(BTN_TIMER) == LOW) {
      Serial.println(">>> ⏲️ TIMER pressed!");
      Serial.println("    Đếm ngược 3 giây...");
      
      // Đếm ngược 3-2-1
      for (int i = 3; i >= 1; i--) {
        Serial.printf("    ⏰ %d...\n", i);
        playTickSound();
        delay(1000);
      }
      
      Serial.println("    📸 CHỤP!");
      playShutterSound();
      
      if (bleKeyboard.isConnected()) {
        bleKeyboard.write(KEY_MEDIA_VOLUME_UP);
        Serial.println("    ✅ Đã chụp");
      } else {
        Serial.println("    ⚠️ Chưa kết nối BLE");
      }
    }
  }
  lastStateTimer = currentState;
}

// ============================================
// 🎵 ============ CÁC ÂM THANH ============
// ============================================

void playTone(int freq, int duration) {
  ledcWriteTone(BUZZER_CHANNEL, freq);
  delay(duration);
  ledcWriteTone(BUZZER_CHANNEL, 0);
}

// 🚀 Âm thanh khởi động (3 nốt tăng dần)
void playBootSound() {
  playTone(NOTE_C5, 100); delay(30);
  playTone(NOTE_E5, 100); delay(30);
  playTone(NOTE_G5, 150);
}

// ✅ Kết nối thành công (2 nốt vui)
void playConnectedSound() {
  playTone(NOTE_E5, 100); delay(50);
  playTone(NOTE_G5, 200);
}

// ❌ Mất kết nối (2 nốt giảm)
void playDisconnectedSound() {
  playTone(NOTE_G4, 150); delay(50);
  playTone(NOTE_C4, 250);
}

// 🔍 Beep tìm kết nối (ngắn)
void playSearchingBeep() {
  playTone(NOTE_A4, 50);
}

// 📸 Tiếng shutter
void playShutterSound() {
  playTone(NOTE_C6, 30); delay(20);
  playTone(NOTE_G5, 50);
}

// 🎥 Bắt đầu quay (beep dài)
void playRecordStartSound() {
  playTone(NOTE_G5, 300);
}

// ⏹️ Dừng quay (2 beep)
void playRecordStopSound() {
  playTone(NOTE_E5, 100); delay(50);
  playTone(NOTE_C5, 150);
}

// 📸📸 Burst click (rất nhanh)
void playBurstClick() {
  playTone(NOTE_C6, 20);
}

// ⏰ Tick đếm ngược
void playTickSound() {
  playTone(NOTE_A4, 80);
}
