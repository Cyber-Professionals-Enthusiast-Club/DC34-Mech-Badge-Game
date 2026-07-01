#pragma once
#include <Arduino.h>

static const uint8_t CPEC_PROTOCOL_VERSION = 1;
static const uint8_t CPEC_FLAG_READY = 1 << 0;
static const uint8_t CPEC_FLAG_IN_BATTLE = 1 << 1;
static const uint8_t CPEC_FLAG_ACCEPTING_CHALLENGES = 1 << 2;

struct CpecAdvertisedPilot {
  uint8_t version = CPEC_PROTOCOL_VERSION;
  uint8_t flags = CPEC_FLAG_READY | CPEC_FLAG_ACCEPTING_CHALLENGES;
  uint8_t chassisId = 0;
  uint8_t factionId = 0;
  String pilotName;
};

String encodeCpecAdvertisement(const CpecAdvertisedPilot &pilot);
bool decodeCpecAdvertisement(const String &data, CpecAdvertisedPilot &pilot);