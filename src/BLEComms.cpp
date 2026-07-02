#include "BLEComms.h"

#include <Arduino.h>
#include <NimBLEDevice.h>
#include "WirelessProtocol.h"

static NearbyBadge nearbyBadges[10];
static int nearbyCount = 0;
static unsigned long lastScanMs = 0;
static const unsigned long SCAN_INTERVAL_MS = 5000;
static const int MAX_NEARBY_BADGES = 10;

void bleSetup(const CpecAdvertisedPilot &pilot) {
  String advName = "C|";
  advName += pilot.chassisId;
  advName += "|";
  advName += pilot.pilotName;

  NimBLEDevice::init(advName.c_str());

  NimBLEAdvertisementData advData;
  advData.setName(advName.c_str());

  NimBLEAdvertising *advertising = NimBLEDevice::getAdvertising();
  advertising->setAdvertisementData(advData);
  advertising->start();

  Serial.print("BLE Advertising: ");
  Serial.println(advName);
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

    String protocolData = device->getName().c_str();
        Serial.print("BLE seen: ");
        Serial.println(protocolData);

    CpecAdvertisedPilot remotePilot;

    if (!decodeCpecAdvertisement(protocolData, remotePilot)) {
      continue;
    }

    Serial.println("CPEC decode OK");

    String displayName = remotePilot.pilotName;
    displayName += " - ";
    displayName += chassisNameFromCode(remotePilot.chassisId);

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