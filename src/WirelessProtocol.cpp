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