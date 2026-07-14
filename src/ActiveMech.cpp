#include "ActiveMech.h"

bool buildActiveMech(
  ActiveMech &mech,
  const PilotProfile &pilot,
  const BadgeConfig &badge,
  const ChassisProfile *chassis,
  const std::vector<WeaponProfile> &weaponProfiles
) {


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

