#pragma once
#include <Arduino.h>
#include "WirelessProtocol.h"

struct NearbyBadge {
  String name;
  int rssi;
};

void bleSetup(const CpecAdvertisedPilot &pilot);
void bleLoop();
void bleAdvertiseChallenge(const CpecAdvertisedPilot &pilot);
bool bleHasIncomingChallenge();
CpecChallengePacket bleGetIncomingChallenge();
void bleClearIncomingChallenge();

int getNearbyBadgeCount();
NearbyBadge getNearbyBadge(int index);