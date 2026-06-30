#pragma once
#include <Arduino.h>

struct NearbyBadge {
  String name;
  int rssi;
};

void bleSetup(const String &advertisingName);
void bleLoop();

int getNearbyBadgeCount();
NearbyBadge getNearbyBadge(int index);