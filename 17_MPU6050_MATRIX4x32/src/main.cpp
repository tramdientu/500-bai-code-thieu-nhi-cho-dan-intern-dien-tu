#include <Arduino.h>
#include <Wire.h>
#include <MPU6050.h>
#include <MD_MAX72xx.h>
#include <SPI.h>

// ===============================================================
// Created by Trạm Điện Tử
// Project JUST FOR FUN 
// ===============================================================
// ===== LED =====
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define CS_PIN 5

MD_MAX72XX mx(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

// ===== MPU =====
MPU6050 mpu;

// ===== PARTICLES =====
#define NUM 60  

float px[NUM], py[NUM];
float vx[NUM], vy[NUM];

// ===== ANGLE =====
float ax, ay, az;
float angleX = 0, angleY = 0;
float prevX = 0, prevY = 0;

// ===== SETUP =====
void setup() {
  Serial.begin(115200);

  Wire.begin(33, 32);
  Wire.setClock(400000);

  mpu.initialize();
  delay(100);

  mx.begin();
  mx.control(MD_MAX72XX::INTENSITY, 3);
  mx.clear();

  // Init particles - toàn bộ màn hình (không cần chừa viền)
  for (int i = 0; i < NUM; i++) {
    px[i] = random(0, 32);
    py[i] = random(0, 8);
    vx[i] = 0;
    vy[i] = 0;
  }
}

// ===== MPU =====
void readMPU() {
  int16_t rax, ray, raz;
  mpu.getAcceleration(&rax, &ray, &raz);

  ax = rax / 16384.0;
  ay = ray / 16384.0;
  az = raz / 16384.0;
}

void calcAngle() {
  float newX = atan2(ay, sqrt(ax * ax + az * az)) * 180 / PI;
  float newY = atan2(ax, sqrt(ay * ay + az * az)) * 180 / PI;

  angleX = 0.85 * prevX + 0.15 * newX;  // Phản hồi nhanh hơn
  angleY = 0.85 * prevY + 0.15 * newY;

  prevX = angleX;
  prevY = angleY;
}

// ===== PARTICLE PHYSICS (như bi) =====
void updateParticles() {
  // Trọng lực từ góc nghiêng
  float gx = angleY * 0.05;  // Mạnh hơn -> bi nặng hơn
  float gy = angleX * 0.05;

  for (int i = 0; i < NUM; i++) {
    // Cộng lực trọng trường
    vx[i] += gx;
    vy[i] += gy;

    // Ma sát (bi lăn trên mặt phẳng)
    vx[i] *= 0.88;
    vy[i] *= 0.88;

    // Giới hạn tốc độ tối đa (bi không bay quá nhanh)
    float maxSpeed = 2.0;
    if (vx[i] > maxSpeed) vx[i] = maxSpeed;
    if (vx[i] < -maxSpeed) vx[i] = -maxSpeed;
    if (vy[i] > maxSpeed) vy[i] = maxSpeed;
    if (vy[i] < -maxSpeed) vy[i] = -maxSpeed;

    // Cập nhật vị trí
    px[i] += vx[i];
    py[i] += vy[i];

    // Va chạm tường - nảy nhẹ như bi thật
    if (px[i] < 0) { px[i] = 0; vx[i] *= -0.3; }
    if (px[i] > 31) { px[i] = 31; vx[i] *= -0.3; }
    if (py[i] < 0) { py[i] = 0; vy[i] *= -0.3; }
    if (py[i] > 7) { py[i] = 7; vy[i] *= -0.3; }
  }

  // Va chạm giữa các hạt (bi đẩy nhau, không chồng lên nhau)
  for (int i = 0; i < NUM; i++) {
    for (int j = i + 1; j < NUM; j++) {
      float dx = px[j] - px[i];
      float dy = py[j] - py[i];
      float dist = sqrt(dx * dx + dy * dy);

      // Nếu 2 bi quá gần nhau (< 1.2 pixel)
      if (dist < 1.2 && dist > 0.01) {
        float overlap = (1.2 - dist) * 0.5;
        float nx = dx / dist;
        float ny = dy / dist;

        // Đẩy 2 bi ra xa nhau
        px[i] -= nx * overlap;
        py[i] -= ny * overlap;
        px[j] += nx * overlap;
        py[j] += ny * overlap;

        // Trao đổi vận tốc (va chạm đàn hồi đơn giản)
        float dvx = vx[i] - vx[j];
        float dvy = vy[i] - vy[j];
        float dot = dvx * nx + dvy * ny;

        if (dot > 0) {
          vx[i] -= 0.5 * dot * nx;
          vy[i] -= 0.5 * dot * ny;
          vx[j] += 0.5 * dot * nx;
          vy[j] += 0.5 * dot * ny;
        }
      }
    }
  }
}

// ===== DRAW (không viền, chỉ có bi) =====
void draw() {
  mx.clear();

  // Vẽ tất cả hạt bi
  for (int i = 0; i < NUM; i++) {
    int ix = constrain((int)round(px[i]), 0, 31);
    int iy = constrain((int)round(py[i]), 0, 7);
    mx.setPoint(iy, ix, true);
  }

  mx.update();
}

// ===== LOOP =====
void loop() {
  readMPU();
  calcAngle();
  updateParticles();
  draw();

  delay(20);  // ~50 FPS
}
