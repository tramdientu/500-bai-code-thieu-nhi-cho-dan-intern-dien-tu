#include <HomeSpan.h>
#include "devices.h"

void addAccessoryInfo(const char* name) {
  new Service::AccessoryInformation();
    new Characteristic::Identify();
    new Characteristic::Name(name);
    new Characteristic::Manufacturer("DIY");
    new Characteristic::Model("SmartHubPRO");
    new Characteristic::SerialNumber("SH-001");
    new Characteristic::FirmwareRevision("3.0");
}
