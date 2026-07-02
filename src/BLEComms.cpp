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
  NimBLEDevice::init("CPEC-BADGE");

  String advString = encodeCpecAdvertisement(pilot);

  NimBLEAdvertisementData advData;
  advData.setName("CPEC-BADGE");
  advData.setServiceData(NimBLEUUID("FFF0"), advString.c_str());

  NimBLEAdvertising *advertising = NimBLEDevice::getAdvertising();
  advertising->setAdvertisementData(advData);
  advertising->start();

  Serial.print("BLE Advertising: ");
  Serial.println(advString);
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
        String protocolData = device->getServiceData().c_str();

            if (device->haveName()) {
                Serial.print("BLE name: ");
                Serial.println(device->getName().c_str());
            }

            //Serial.print("Service data: [");
            //Serial.print(protocolData);
            //Serial.println("]");

        CpecAdvertisedPilot remotePilot;

            if (decodeCpecAdvertisement(protocolData, remotePilot)) {
                Serial.println("CPEC decode OK");

                String displayName = remotePilot.pilotName;
                displayName += " - ";
                displayName += chassisNameFromCode(remotePilot.chassisId);

                if (nearbyCount < MAX_NEARBY_BADGES) {
                    nearbyBadges[nearbyCount].name = displayName;
                    nearbyBadges[nearbyCount].rssi = device->getRSSI();
                    nearbyCount++;
                }

            continue;
}

        if (device->haveName()) {
        String devName = device->getName().c_str();

        if (devName == "CPEC-BADGE" && nearbyCount < MAX_NEARBY_BADGES) {
            nearbyBadges[nearbyCount].name = "Unknown CPEC Badge";
            nearbyBadges[nearbyCount].rssi = device->getRSSI();
            nearbyCount++;
        }
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