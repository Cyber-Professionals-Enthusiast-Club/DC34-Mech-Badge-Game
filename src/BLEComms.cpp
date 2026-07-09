#include "BLEComms.h"

#include <Arduino.h>
#include <NimBLEDevice.h>
#include "WirelessProtocol.h"

static NearbyBadge nearbyBadges[10];
static int nearbyCount = 0;
static unsigned long lastScanMs = 0;
static const unsigned long SCAN_INTERVAL_MS = 5000;
static const int MAX_NEARBY_BADGES = 10;
static CpecAdvertisedPilot localAdvertisedPilot;
static bool incomingChallengeAvailable = false;
static CpecChallengePacket incomingChallenge;

void bleSetup(const CpecAdvertisedPilot &pilot) {
  localAdvertisedPilot = pilot;
  
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

void bleAdvertiseChallenge(const CpecAdvertisedPilot &pilot) {
  String challengeName = encodeChallengePacket(pilot);

  NimBLEAdvertising *advertising = NimBLEDevice::getAdvertising();
  advertising->stop();

  NimBLEAdvertisementData advData;
  advData.setName(challengeName.c_str());

  advertising->setAdvertisementData(advData);
  advertising->start();

  Serial.print("BLE Challenge Advertising: ");
  Serial.println(challengeName);
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

    CpecChallengePacket challenge;

    if (decodeChallengePacket(protocolData, challenge)) {
    incomingChallenge = challenge;
    incomingChallengeAvailable = true;
        Serial.print("BLE seen: ");
        Serial.println(protocolData);
    Serial.println("Incoming challenge decoded");
    continue;
    }

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

bool bleHasIncomingChallenge() {
  return incomingChallengeAvailable;
}

CpecChallengePacket bleGetIncomingChallenge() {
  return incomingChallenge;
}

void bleClearIncomingChallenge() {
  incomingChallengeAvailable = false;
}