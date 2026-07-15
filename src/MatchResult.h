#pragma once

#include <Arduino.h>

struct MatchResult {
  int version = 1;

  String localPilotId;
  String localPilotName;

  String remotePilotId;
  String remotePilotName;

  String result;  // "W" or "L"

  int rounds = 0;

  String localChassisId;
  String remoteChassisId;
};

String encodeMatchResult(const MatchResult &result);