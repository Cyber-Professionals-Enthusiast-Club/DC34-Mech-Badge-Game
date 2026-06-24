#include "ActiveMech.h"

bool buildActiveMech(
  ActiveMech &mech,
  const PilotProfile &pilot,
  const BadgeConfig &badge,
  const ChassisProfile *chassis,
  const std::vector<WeaponProfile> &weaponProfiles
) {
  if (!chassis) {
    Serial.println("buildActiveMech failed: chassis is null.");
    return false;
  }

  mech.pilot = pilot;
  mech.badge = badge;
  mech.chassis = *chassis;
  mech.currentHeat = 0;

  mech.locations.clear();
  mech.weapons.clear();

  for (const LocationHP &loc : chassis->locations) {
    ActiveLocation activeLoc;
    activeLoc.id = loc.id;
    activeLoc.maxArmor = loc.armor;
    activeLoc.currentArmor = loc.armor;
    activeLoc.maxStructure = loc.structure;
    activeLoc.currentStructure = loc.structure;

    mech.locations.push_back(activeLoc);
  }

  for (const WeaponSlot &slot : chassis->baseLoadout) {
    const WeaponProfile *weapon =
        findWeaponById(weaponProfiles, slot.baseWeaponId);

    if (!weapon) {
      Serial.print("Missing weapon profile for slot ");
      Serial.print(slot.slot);
      Serial.print(": ");
      Serial.println(slot.baseWeaponId);
      continue;
    }

    ActiveWeapon activeWeapon;
    activeWeapon.slot = slot.slot;
    activeWeapon.weapon = *weapon;
    activeWeapon.isSaoOverride = false;

    activeWeapon.maxAmmo = -1;
    activeWeapon.currentAmmo = -1;

   if (weapon->category == "missile") {
  activeWeapon.maxAmmo = 6;
  activeWeapon.currentAmmo = 6;
}

if (weapon->category == "ballistic") {
  activeWeapon.maxAmmo = 5;
  activeWeapon.currentAmmo = 5;
}

mech.weapons.push_back(activeWeapon);
  }

  return mech.locations.size() > 0 && mech.weapons.size() > 0;
}

void printActiveMech(const ActiveMech &mech) {
  Serial.println();
  Serial.println("=== ACTIVE MECH OBJECT ===");

  Serial.print("Pilot: ");
  Serial.println(mech.pilot.pilotName);

  Serial.print("Faction: ");
  Serial.println(mech.pilot.faction);

  Serial.print("Badge: ");
  Serial.println(mech.badge.badgeId);

  Serial.print("Chassis: ");
  Serial.print(mech.chassis.displayName);
  Serial.print(" / ");
  Serial.println(mech.chassis.mechClass);

  Serial.print("Initiative: ");
  Serial.println(mech.chassis.initiative);

  Serial.print("Mobility: ");
  Serial.println(mech.chassis.mobility);

  Serial.print("Heat: ");
  Serial.print(mech.currentHeat);
  Serial.print("/");
  Serial.println(mech.chassis.heatCapacity);

  Serial.println();
  Serial.println("Locations:");
  for (const ActiveLocation &loc : mech.locations) {
    Serial.print("  ");
    Serial.print(loc.id);
    Serial.print(" A:");
    Serial.print(loc.currentArmor);
    Serial.print("/");
    Serial.print(loc.maxArmor);
    Serial.print(" S:");
    Serial.print(loc.currentStructure);
    Serial.print("/");
    Serial.println(loc.maxStructure);
  }

  Serial.println();
  Serial.println("Weapons:");
  for (const ActiveWeapon &w : mech.weapons) {
    Serial.print("  Slot ");
    Serial.print(w.slot);
    Serial.print(": ");
    Serial.print(w.weapon.displayName);

    if (w.isSaoOverride) {
      Serial.print(" [SAO]");
    }

    Serial.print(" DMG:");
    Serial.print(w.weapon.damage);
    Serial.print(" HEAT:");
    Serial.print(w.weapon.heat);
    Serial.print(" ACC:");
    Serial.print(w.weapon.accuracyModifier);
    Serial.print(" RNG:");
    Serial.println(w.weapon.range);
  }

  Serial.println("==========================");
}