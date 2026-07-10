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
static bool incomingAcceptAvailable = false;
static CpecAcceptPacket incomingAccept;
static bool incomingTurnAvailable = false;
static CpecTurnPacket incomingTurn;

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

void bleAdvertiseAccept(const CpecAdvertisedPilot &pilot) {
  String acceptName = encodeAcceptPacket(pilot);

  NimBLEAdvertising *advertising = NimBLEDevice::getAdvertising();
  advertising->stop();

  NimBLEAdvertisementData advData;
  advData.setName(acceptName.c_str());

  advertising->setAdvertisementData(advData);
  advertising->start();

  Serial.print("BLE Accept Advertising: ");
  Serial.println(acceptName);
}

void bleAdvertiseTurn(int round, int weaponSlot) {
  String turnName = encodeTurnPacket(round, weaponSlot);

  NimBLEAdvertising *advertising = NimBLEDevice::getAdvertising();
  advertising->stop();

  NimBLEAdvertisementData advData;
  advData.setName(turnName.c_str());

  advertising->setAdvertisementData(advData);
  advertising->start();

  Serial.print("BLE Turn Advertising: ");
  Serial.println(turnName);
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
    
    CpecAcceptPacket accept;
    if (decodeAcceptPacket(protocolData, accept)) {
    incomingAccept = accept;
    incomingAcceptAvailable = true;
    Serial.println("Incoming accept decoded");
    continue;
    }

    CpecTurnPacket turn;

    if (decodeTurnPacket(protocolData, turn)) {
    incomingTurn = turn;
    incomingTurnAvailable = true;

    Serial.print("Incoming turn decoded: round ");
    Serial.print(turn.round);
    Serial.print(", weapon ");
    Serial.println(turn.weaponSlot);

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

bool bleHasIncomingAccept() {
  return incomingAcceptAvailable;
}

CpecAcceptPacket bleGetIncomingAccept() {
  return incomingAccept;
}

void bleClearIncomingAccept() {
  incomingAcceptAvailable = false;
}

bool bleHasIncomingTurn() {
  return incomingTurnAvailable;
}

CpecTurnPacket bleGetIncomingTurn() {
  return incomingTurn;
}

void bleClearIncomingTurn() {
  incomingTurnAvailable = false;
}