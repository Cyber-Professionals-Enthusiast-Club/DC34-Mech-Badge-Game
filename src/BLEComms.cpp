#include "BLEComms.h"

#include <Arduino.h>
#include <NimBLEDevice.h>

static NearbyBadge nearbyBadges[10];
static int nearbyCount = 0;
static unsigned long lastScanMs = 0;
static const unsigned long SCAN_INTERVAL_MS = 5000;
static const int MAX_NEARBY_BADGES = 10;

void bleSetup(const String &advertisingName) {
  NimBLEDevice::init(advertisingName.c_str());

  NimBLEAdvertisementData advData;
  advData.setName(advertisingName.c_str());

  NimBLEAdvertising *advertising = NimBLEDevice::getAdvertising();
  advertising->setAdvertisementData(advData);
  advertising->start();

  Serial.print("BLE Advertising Started: ");
  Serial.println(advertisingName);
}

void bleLoop() {
  if (millis() - lastScanMs < SCAN_INTERVAL_MS) {
    return;
  }

  lastScanMs = millis();

  nearbyCount = 0;

    NimBLEScan *scan = NimBLEDevice::getScan();
    scan->setActiveScan(true);
    scan->setInterval(45);
    scan->setWindow(15);

    NimBLEScanResults results = scan->getResults(2000, false);

  for (int i = 0; i < results.getCount(); i++) {
    const NimBLEAdvertisedDevice *device = results.getDevice(i);

    if (!device->haveName()) {
      continue;
    }

    String name = device->getName().c_str();
    if (!name.startsWith("CPEC:")) {
    continue;

    
}

    // For now, only collect CPEC-style badge names.
    // This will catch names like "Pathfinder - HAWK".
    if (name.length() == 0) {
      continue;
    }
    String displayName = name.substring(5);

    if (nearbyCount < MAX_NEARBY_BADGES) {
    nearbyBadges[nearbyCount].name = displayName;
    nearbyBadges[nearbyCount].rssi = device->getRSSI();
    nearbyCount++;
}
  }

  scan->clearResults();
}

int getNearbyBadgeCount() {
  return nearbyCount;
}

NearbyBadge getNearbyBadge(int index) {
  if (index < 0 || index >= nearbyCount) {
    NearbyBadge empty;
    empty.name = "";
    empty.rssi = 0;
    return empty;
  }

  return nearbyBadges[index];
}