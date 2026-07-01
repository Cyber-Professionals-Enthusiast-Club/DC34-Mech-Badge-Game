#pragma once
#include <Arduino.h>
#include "WirelessProtocol.h"

struct NearbyBadge {
  String name;
  int rssi;
};

void bleSetup(const CpecAdvertisedPilot &pilot);
void bleLoop();

int getNearbyBadgeCount();
NearbyBadge getNearbyBadge(int index);