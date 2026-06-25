#pragma once
#include <Arduino.h>
#include <vector>

struct PilotProfile {
  int schemaVersion = 1;
  String pilotId;
  String pilotName;
  String faction;
  int xp = 0;
  int level = 1;
  int skillPoints = 0;
  int wins = 0, losses = 0, kills = 0, deaths = 0, battles = 0;
};

struct BadgeConfig {
  int schemaVersion = 1;
  String badgeId;
  String chassisId;
  String salt;
};

struct LocationHP {
  String id;
  int armor = 0;
  int structure = 0;
};

struct WeaponSlot {
  int slot = 0;
  String baseWeaponId;
};

struct ChassisProfile {
  String id;
  String displayName;
  String mechClass;
  int initiative = 0;
  int mobility = 0;
  int heatCapacity = 0;
  int saoSlots = 0;
  std::vector<LocationHP> locations;
  std::vector<WeaponSlot> baseLoadout;
};

struct WeaponProfile {
  String id;
  String displayName;
  String category;
  String source;
  int damage = 0;
  int heat = 0;
  int accuracyModifier = 0;
  String range;
};

struct FactionProfile {
  String id;
  String displayName;
  String description;
};

bool mountBadgeFilesystem();
bool loadPilotProfile(PilotProfile &pilot);
bool savePilotProfile(const PilotProfile &pilot);
bool loadBadgeConfig(BadgeConfig &badge);
bool loadChassisProfiles(std::vector<ChassisProfile> &chassisList);
bool loadWeaponProfiles(std::vector<WeaponProfile> &weaponList);
bool loadFactionProfiles(std::vector<FactionProfile> &factionList);

const ChassisProfile* findChassisById(const std::vector<ChassisProfile> &list, const String &id);
const WeaponProfile* findWeaponById(const std::vector<WeaponProfile> &list, const String &id);
const FactionProfile* findFactionById(const std::vector<FactionProfile> &list, const String &id);

void printLoadedGameSummary(
  const PilotProfile &pilot,
  const BadgeConfig &badge,
  const ChassisProfile *chassis,
  const std::vector<WeaponProfile> &weapons,
  const std::vector<FactionProfile> &factions
);
