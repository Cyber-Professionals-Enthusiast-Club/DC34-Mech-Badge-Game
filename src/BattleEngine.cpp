#include "BattleEngine.h"
#include <Arduino.h>

static ActiveMech *localBattleMech = nullptr;
static ActiveMech *remoteBattleMech = nullptr;

static BattleTurn currentTurn;

static ActiveLocation *findLocation(
    ActiveMech *mech,
    const String &locationId
) {
  if (mech == nullptr) {
    return nullptr;
  }

  for (ActiveLocation &location : mech->locations) {
    if (location.id == locationId) {
      return &location;
    }
  }

  return nullptr;
}

static void applyDamageToLocation(
    ActiveLocation &location,
    int damage
) {
  if (damage <= 0) {
    return;
  }

  int armorDamage = min(damage, location.currentArmor);
  location.currentArmor -= armorDamage;
  damage -= armorDamage;

  if (damage > 0) {
    int structureDamage = min(damage, location.currentStructure);
    location.currentStructure -= structureDamage;
  }
}

void startMultiplayerBattle(
    ActiveMech &localMech,
    ActiveMech &remoteMech
) {
  localBattleMech = &localMech;
  remoteBattleMech = &remoteMech;

  currentTurn.round = 1;
  currentTurn.localWeaponSlot = -1;
  currentTurn.remoteWeaponSlot = -1;
  currentTurn.localSubmitted = false;
  currentTurn.remoteSubmitted = false;
}

void submitLocalTurn(int weaponSlot) {
  currentTurn.localWeaponSlot = weaponSlot;
  currentTurn.localSubmitted = true;
}

void submitRemoteTurn(int weaponSlot) {
  currentTurn.remoteWeaponSlot = weaponSlot;
  currentTurn.remoteSubmitted = true;
}

bool battleTurnReady() {
  return currentTurn.localSubmitted &&
         currentTurn.remoteSubmitted;
}

void resolveBattleTurn() {
  if (!battleTurnReady()) {
    return;
  }

  if (localBattleMech == nullptr ||
      remoteBattleMech == nullptr) {
    Serial.println("Battle resolve failed: mech pointers are null");
    return;
  }

  int localWeaponSlot = currentTurn.localWeaponSlot;
  int remoteWeaponSlot = currentTurn.remoteWeaponSlot;

  bool localWeaponValid =
      localWeaponSlot >= 0 &&
      localWeaponSlot < localBattleMech->weapons.size();

  bool remoteWeaponValid =
      remoteWeaponSlot >= 0 &&
      remoteWeaponSlot < remoteBattleMech->weapons.size();

  ActiveLocation *localCT =
      findLocation(localBattleMech, "CT");

  ActiveLocation *remoteCT =
      findLocation(remoteBattleMech, "CT");

  if (localWeaponValid && remoteCT != nullptr) {
    int damage =
        localBattleMech
            ->weapons[localWeaponSlot]
            .weapon
            .damage;

    applyDamageToLocation(*remoteCT, damage);

    Serial.print("Local attack dealt ");
    Serial.print(damage);
    Serial.println(" damage to remote CT");
  }

  if (remoteWeaponValid && localCT != nullptr) {
    int damage =
        remoteBattleMech
            ->weapons[remoteWeaponSlot]
            .weapon
            .damage;

    applyDamageToLocation(*localCT, damage);

    Serial.print("Remote attack dealt ");
    Serial.print(damage);
    Serial.println(" damage to local CT");
  }

  currentTurn.round++;

  currentTurn.localWeaponSlot = -1;
  currentTurn.remoteWeaponSlot = -1;
  currentTurn.localSubmitted = false;
  currentTurn.remoteSubmitted = false;
}
const BattleTurn &getBattleTurn() {
  return currentTurn;
}

int getBattleRound() {
  return currentTurn.round;
}