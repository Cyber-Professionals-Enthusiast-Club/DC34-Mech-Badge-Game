#include "WirelessProtocol.h"

String encodeCpecAdvertisement(const CpecAdvertisedPilot &pilot) {
  String out = "CPEC|";
  out += String(pilot.version);
  out += "|";
  out += String(pilot.flags);
  out += "|";
  out += String(pilot.chassisId);
  out += "|";
  out += String(pilot.factionId);
  out += "|";

  String name = pilot.pilotName;
  if (name.length() > 16) {
    name = name.substring(0, 16);
  }

  out += name;
  return out;
}

bool decodeCpecAdvertisement(const String &data, CpecAdvertisedPilot &pilot) {
  // New compact format:
  // C|3|STEVE
  if (data.startsWith("C|")) {
    int p1 = data.indexOf('|', 2);

    if (p1 < 0) {
      return false;
    }

    pilot.version = CPEC_PROTOCOL_VERSION;
    pilot.flags = CPEC_FLAG_READY | CPEC_FLAG_ACCEPTING_CHALLENGES;
    pilot.chassisId = data.substring(2, p1).toInt();
    pilot.factionId = 0;
    pilot.pilotName = data.substring(p1 + 1);

    return true;
  }

  // Older full format:
  // CPEC|1|5|3|0|STEVE
  if (!data.startsWith("CPEC|")) {
    return false;
  }

  int p1 = data.indexOf('|', 5);
  int p2 = data.indexOf('|', p1 + 1);
  int p3 = data.indexOf('|', p2 + 1);
  int p4 = data.indexOf('|', p3 + 1);

  if (p1 < 0 || p2 < 0 || p3 < 0 || p4 < 0) {
    return false;
  }

  pilot.version = data.substring(5, p1).toInt();
  pilot.flags = data.substring(p1 + 1, p2).toInt();
  pilot.chassisId = data.substring(p2 + 1, p3).toInt();
  pilot.factionId = data.substring(p3 + 1, p4).toInt();
  pilot.pilotName = data.substring(p4 + 1);

  return true;
}

String encodeChallengePacket(const CpecAdvertisedPilot &pilot) {
  String out = "Q|";
  out += pilot.pilotName;
  out += "|";
  out += pilot.chassisId;
  return out;
}

bool decodeChallengePacket(const String &data, CpecChallengePacket &packet) {
  if (!data.startsWith("Q|")) {
    return false;
  }

  int p1 = data.indexOf('|', 2);

  if (p1 < 0) {
    return false;
  }

  packet.challengerName = data.substring(2, p1);
  packet.chassisId = data.substring(p1 + 1).toInt();

  return true;
}

uint8_t chassisCodeFromId(const String &chassisId) {
  if (chassisId == "PEST") return 1;
  if (chassisId == "CREEPER") return 2;
  if (chassisId == "PATHFINDER") return 3;
  if (chassisId == "DOZER") return 4;

  return 0;
}

String chassisNameFromCode(uint8_t code) {
  switch (code) {
    case 1: return "Pest";
    case 2: return "Creeper";
    case 3: return "Pathfinder";
    case 4: return "Dozer";
    default: return "Unknown";
  }
}
