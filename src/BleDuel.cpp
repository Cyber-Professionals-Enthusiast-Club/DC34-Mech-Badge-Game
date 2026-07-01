#include "BleDuel.h"
#include <Arduino.h>
#include <NimBLEDevice.h>

void bleDuelSetup() {

    NimBLEDevice::init("BOBS FIGHTIN' ROBIT");

    NimBLEAdvertising *advertising =
        NimBLEDevice::getAdvertising();

    advertising->start();

    Serial.println("Advertising started.");
}

void bleDuelLoop() {
  // BLE test loop placeholder
}