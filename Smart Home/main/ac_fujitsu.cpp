#include "devices.h"

DieuHoaFujitsu::DieuHoaFujitsu() : Service::HeaterCooler() {
  active       = new Characteristic::Active(0);
  currentState = new Characteristic::CurrentHeaterCoolerState(0);
  targetState  = new Characteristic::TargetHeaterCoolerState(2);
  currentTemp  = new Characteristic::CurrentTemperature(25);
  currentTemp->setRange(-40, 80);

  coolTemp = new Characteristic::CoolingThresholdTemperature(25);
  coolTemp->setRange(16, 30, 1);
  heatTemp = new Characteristic::HeatingThresholdTemperature(22);
  heatTemp->setRange(16, 30, 1);

  rotSpeed = new Characteristic::RotationSpeed(50);
  rotSpeed->setRange(0, 100, 25);
  swingMode = new Characteristic::SwingMode(0);

  fujitsuAC.begin();
  fujitsuAC.setModel(ARRAH2E);
}

boolean DieuHoaFujitsu::update() {
  bool isOn = active->getNewVal();

  if (isOn != acLastActive && millis() < acLockUntil) {
    unsigned long remain = acLockUntil - millis();
    Serial.printf("[LOCK] Dieu Hoa đang khóa! Còn %lums\n", remain);
    active->setVal(acLastActive);
    return false;
  }

  if (!isOn) {
    fujitsuAC.off();
    fujitsuAC.send();
    currentState->setVal(0);
    acLastActive = false;
    acLockUntil = millis() + 1500;
    Serial.println("[AC] TAT");
    return true;
  }

  int mode = targetState->getNewVal();
  int acMode, curState;
  float setTemp;

  switch (mode) {
    case 0:
      acMode   = kFujitsuAcModeAuto;
      setTemp  = coolTemp->getNewVal<float>();
      curState = (g_temp > setTemp) ? 3 : 2;
      break;
    case 1:
      acMode   = kFujitsuAcModeHeat;
      setTemp  = heatTemp->getNewVal<float>();
      curState = 2;
      break;
    default:
      acMode   = kFujitsuAcModeCool;
      setTemp  = coolTemp->getNewVal<float>();
      curState = 3;
      break;
  }

  int fan = rotSpeed->getNewVal();
  int acFan;
  if (fan == 0)        acFan = kFujitsuAcFanAuto;
  else if (fan <= 25)  acFan = kFujitsuAcFanLow;
  else if (fan <= 50)  acFan = kFujitsuAcFanMed;
  else                 acFan = kFujitsuAcFanHigh;

  int swing = swingMode->getNewVal();
  int acSwing = swing ? kFujitsuAcSwingVert : kFujitsuAcSwingOff;

  fujitsuAC.setMode(acMode);
  fujitsuAC.setTemp((uint8_t)setTemp);
  fujitsuAC.setFanSpeed(acFan);
  fujitsuAC.setSwing(acSwing);
  fujitsuAC.on();
  fujitsuAC.send();

  currentState->setVal(curState);
  acLastActive = true;
  acLockUntil = millis() + 1500;

  Serial.printf("[AC] BAT | Mode:%d | Temp:%.0f | Fan:%d | Swing:%d\n",
                mode, setTemp, fan, swing);
  return true;
}

void DieuHoaFujitsu::loop() {
  if (millis() - lastUpdate > 10000) {
    lastUpdate = millis();
    currentTemp->setVal(g_temp);
  }
}
