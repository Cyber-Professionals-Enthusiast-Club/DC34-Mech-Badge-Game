#include "ProfileLoader.h"
#include <ArduinoJson.h>
#include <LittleFS.h>

static const char *PILOT_PROFILE_PATH = "/pilot_profile.json";
static const char *BADGE_CONFIG_PATH = "/badge_config.json";
static const char *CHASSIS_PROFILES_PATH = "/chassis_profiles.json";
static const char *WEAPON_PROFILES_PATH = "/weapon_profiles.json";
static const char *FACTION_PROFILES_PATH = "/faction_profiles.json";

bool mountBadgeFilesystem() {
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS mount failed.");
    return false;
  }
  Serial.println("LittleFS mounted.");
  return true;
}

bool loadPilotProfile(PilotProfile &pilot) {
  if (!LittleFS.exists(PILOT_PROFILE_PATH)) {
    Serial.println("pilot_profile.json missing.");
    return false;
  }

  File file = LittleFS.open(PILOT_PROFILE_PATH, "r");
  if (!file) {
    Serial.println("Could not open pilot_profile.json.");
    return false;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, file);
  file.close();

  if (err) {
    Serial.print("pilot_profile.json parse failed: ");
    Serial.println(err.c_str());
    return false;
  }

  pilot.schemaVersion = doc["schema_version"] | 1;
  pilot.pilotId = doc["pilot_id"] | "";
  pilot.pilotName = doc["pilot_name"] | "";
  pilot.faction = doc["faction"] | "";
  pilot.xp = doc["xp"] | 0;
  pilot.level = doc["level"] | 1;
  pilot.skillPoints = doc["skill_points"] | 0;

  JsonObject stats = doc["stats"];
  pilot.wins = stats["wins"] | 0;
  pilot.losses = stats["losses"] | 0;
  pilot.kills = stats["kills"] | 0;
  pilot.deaths = stats["deaths"] | 0;
  pilot.battles = stats["battles"] | 0;

  return true;
}

bool savePilotProfile(const PilotProfile &pilot) {
  JsonDocument doc;
  doc["schema_version"] = pilot.schemaVersion;
  doc["pilot_id"] = pilot.pilotId;
  doc["pilot_name"] = pilot.pilotName;
  doc["faction"] = pilot.faction;
  doc["xp"] = pilot.xp;
  doc["level"] = pilot.level;
  doc["skill_points"] = pilot.skillPoints;

  JsonObject stats = doc["stats"].to<JsonObject>();
  stats["wins"] = pilot.wins;
  stats["losses"] = pilot.losses;
  stats["kills"] = pilot.kills;
  stats["deaths"] = pilot.deaths;
  stats["battles"] = pilot.battles;

  doc["skills"].to<JsonArray>();

  File file = LittleFS.open(PILOT_PROFILE_PATH, "w");
  if (!file) {
    Serial.println("Could not open pilot_profile.json for writing.");
    return false;
  }

  serializeJsonPretty(doc, file);
  file.close();
  return true;
}

bool loadBadgeConfig(BadgeConfig &badge) {
  if (!LittleFS.exists(BADGE_CONFIG_PATH)) {
    Serial.println("badge_config.json missing.");
    return false;
  }

  File file = LittleFS.open(BADGE_CONFIG_PATH, "r");
  if (!file) {
    Serial.println("Could not open badge_config.json.");
    return false;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, file);
  file.close();

  if (err) {
    Serial.print("badge_config.json parse failed: ");
    Serial.println(err.c_str());
    return false;
  }

  Serial.print("Badge loaded: ");
  Serial.println(badge.badgeId);

  Serial.print("Chassis ID loaded: [");
  Serial.print(badge.chassisId);
  Serial.println("]");

  badge.schemaVersion = doc["schema_version"] | 1;
  badge.badgeId = doc["badge_id"] | "";
  badge.chassisId = doc["chassis"] | "";
  badge.salt = doc["salt"] | "";

  return badge.badgeId.length() > 0 && badge.chassisId.length() > 0;

  Serial.print("Loaded chassis ID: ");
  Serial.println(badge.chassisId);
}

bool loadChassisProfiles(std::vector<ChassisProfile> &chassisList) {
  chassisList.clear();

  if (!LittleFS.exists(CHASSIS_PROFILES_PATH)) {
    Serial.println("chassis_profiles.json missing.");
    return false;
  }

  File file = LittleFS.open(CHASSIS_PROFILES_PATH, "r");
  if (!file) {
    Serial.println("Could not open chassis_profiles.json.");
    return false;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, file);
  file.close();

  if (err) {
    Serial.print("chassis_profiles.json parse failed: ");
    Serial.println(err.c_str());
    return false;
  }

  JsonArray arr = doc["chassis"].as<JsonArray>();
  for (JsonObject item : arr) {
    ChassisProfile c;
    c.id = item["id"] | "";
    c.displayName = item["display_name"] | "";
    c.mechClass = item["class"] | "";
    c.initiative = item["initiative"] | 0;
    c.mobility = item["mobility"] | 0;
    c.heatCapacity = item["heat_capacity"] | 0;
    c.saoSlots = item["sao_slots"] | 0;

    JsonObject locations = item["locations"];
    for (JsonPair kv : locations) {
      LocationHP loc;
      loc.id = kv.key().c_str();
      loc.armor = kv.value()["armor"] | 0;
      loc.structure = kv.value()["structure"] | 0;
      c.locations.push_back(loc);
    }

    JsonArray loadout = item["base_loadout"].as<JsonArray>();
    for (JsonObject slotItem : loadout) {
      WeaponSlot s;
      s.slot = slotItem["slot"] | 0;
      s.baseWeaponId = slotItem["base_weapon"] | "";
      c.baseLoadout.push_back(s);
    }

    if (c.id.length() > 0) chassisList.push_back(c);
  }

  return chassisList.size() > 0;
}

bool loadWeaponProfiles(std::vector<WeaponProfile> &weaponList) {
  weaponList.clear();

  if (!LittleFS.exists(WEAPON_PROFILES_PATH)) {
    Serial.println("weapon_profiles.json missing.");
    return false;
  }

  File file = LittleFS.open(WEAPON_PROFILES_PATH, "r");
  if (!file) {
    Serial.println("Could not open weapon_profiles.json.");
    return false;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, file);
  file.close();

  if (err) {
    Serial.print("weapon_profiles.json parse failed: ");
    Serial.println(err.c_str());
    return false;
  }

  JsonArray arr = doc["weapons"].as<JsonArray>();
  for (JsonObject item : arr) {
    WeaponProfile w;
    w.id = item["id"] | "";
    w.displayName = item["display_name"] | "";
    w.category = item["category"] | "";
    w.source = item["source"] | "";
    w.damage = item["damage"] | 0;
    w.heat = item["heat"] | 0;
    w.accuracyModifier = item["accuracy_modifier"] | 0;
    w.range = item["range"] | "";
    if (w.id.length() > 0) weaponList.push_back(w);
  }

  return weaponList.size() > 0;
}

bool loadFactionProfiles(std::vector<FactionProfile> &factionList) {
  factionList.clear();

  if (!LittleFS.exists(FACTION_PROFILES_PATH)) {
    Serial.println("faction_profiles.json missing.");
    return false;
  }

  File file = LittleFS.open(FACTION_PROFILES_PATH, "r");
  if (!file) {
    Serial.println("Could not open faction_profiles.json.");
    return false;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, file);
  file.close();

  if (err) {
    Serial.print("faction_profiles.json parse failed: ");
    Serial.println(err.c_str());
    return false;
  }

  JsonArray arr = doc["factions"].as<JsonArray>();
  for (JsonObject item : arr) {
    FactionProfile f;
    f.id = item["id"] | "";
    f.displayName = item["display_name"] | "";
    f.description = item["description"] | "";
    if (f.id.length() > 0) factionList.push_back(f);
  }

  return factionList.size() > 0;
}

const ChassisProfile* findChassisById(const std::vector<ChassisProfile> &list, const String &id) {
  for (const ChassisProfile &c : list) if (c.id == id) return &c;
  return nullptr;
}

const WeaponProfile* findWeaponById(const std::vector<WeaponProfile> &list, const String &id) {
  for (const WeaponProfile &w : list) if (w.id == id) return &w;
  return nullptr;
}

const FactionProfile* findFactionById(const std::vector<FactionProfile> &list, const String &id) {
  for (const FactionProfile &f : list) if (f.id == id) return &f;
  return nullptr;
}

void printLoadedGameSummary(
  const PilotProfile &pilot,
  const BadgeConfig &badge,
  const ChassisProfile *chassis,
  const std::vector<WeaponProfile> &weapons,
  const std::vector<FactionProfile> &factions
) {
  Serial.println();
  Serial.println("=== LOADED GAME SUMMARY ===");

  Serial.print("Pilot: ");
  Serial.println(pilot.pilotName);

  const FactionProfile *faction = findFactionById(factions, pilot.faction);
  Serial.print("Faction: ");
  if (faction) Serial.println(faction->displayName);
  else Serial.println(pilot.faction);

  Serial.print("Badge ID: ");
  Serial.println(badge.badgeId);

  Serial.print("Chassis ID: ");
  Serial.println(badge.chassisId);

  if (!chassis) {
    Serial.println("ERROR: Chassis not found.");
    return;
  }

  Serial.print("Chassis: ");
  Serial.print(chassis->displayName);
  Serial.print(" / ");
  Serial.println(chassis->mechClass);

  Serial.print("Initiative: ");
  Serial.println(chassis->initiative);
  Serial.print("Mobility: ");
  Serial.println(chassis->mobility);
  Serial.print("Heat Capacity: ");
  Serial.println(chassis->heatCapacity);

  Serial.println();
  Serial.println("Locations:");
  for (const LocationHP &loc : chassis->locations) {
    Serial.print("  ");
    Serial.print(loc.id);
    Serial.print(" A:");
    Serial.print(loc.armor);
    Serial.print(" S:");
    Serial.println(loc.structure);
  }

  Serial.println();
  Serial.println("Base Loadout:");
  for (const WeaponSlot &slot : chassis->baseLoadout) {
    const WeaponProfile *weapon = findWeaponById(weapons, slot.baseWeaponId);

    Serial.print("  Slot ");
    Serial.print(slot.slot);
    Serial.print(": ");

    if (weapon) {
      Serial.print(weapon->displayName);
      Serial.print(" DMG:");
      Serial.print(weapon->damage);
      Serial.print(" HEAT:");
      Serial.print(weapon->heat);
      Serial.print(" ACC:");
      Serial.print(weapon->accuracyModifier);
      Serial.print(" RNG:");
      Serial.println(weapon->range);
    } else {
      Serial.print(slot.baseWeaponId);
      Serial.println(" [weapon missing]");
    }
  }

  Serial.println("===========================");
}
