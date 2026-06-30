#include "BLEComms.h"
#include <Arduino.h>
#include <NimBLEDevice.h>

void bleSetup() {
  NimBLEDevice::init("CPEC-MECH");

  NimBLEAdvertising *advertising = NimBLEDevice::getAdvertising();

  NimBLEAdvertisementData advData;
  advData.setName("CPEC-MECH");
  advertising->setAdvertisementData(advData);

  advertising->start();

  Serial.println("BLE Advertising Started: CPEC-MECH");
}

void bleLoop() {
  // Empty for Revision 1
}