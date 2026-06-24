#pragma once

#include <Arduino.h>
#include <vector>
#include "ProfileLoader.h"

struct ActiveLocation {
  String id;
  int maxArmor = 0;
  int currentArmor = 0;
  int maxStructure = 0;
  int currentStructure = 0;
};

struct ActiveWeapon {
  int slot = 0;
  WeaponProfile weapon;
  bool isSaoOverride = false;

  int maxAmmo = -1;
  int currentAmmo = -1;
};

struct ActiveMech {
  PilotProfile pilot;
  BadgeConfig badge;
  ChassisProfile chassis;

  std::vector<ActiveLocation> locations;
  std::vector<ActiveWeapon> weapons;

  int currentHeat = 0;

  bool shutdown = false;
  int shutdownTurnsRemaining = 0;
};

bool buildActiveMech(
  ActiveMech &mech,
  const PilotProfile &pilot,
  const BadgeConfig &badge,
  const ChassisProfile *chassis,
  const std::vector<WeaponProfile> &weaponProfiles
);

void printActiveMech(const ActiveMech &mech);