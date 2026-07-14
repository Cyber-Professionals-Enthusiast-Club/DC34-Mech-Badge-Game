#include "BattleEngine.h"
#include <Arduino.h>

static ActiveMech *localBattleMech = nullptr;
static ActiveMech *remoteBattleMech = nullptr;

static BattleTurn currentTurn;
static BattleRoundResult lastBattleResult;

static int calculateTargetNumber() {
  return 7;
}

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

  lastBattleResult = BattleRoundResult();
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

static uint32_t hashPilotName(const String &name) {
  uint32_t hash = 2166136261UL;

  for (size_t i = 0; i < name.length(); i++) {
    hash ^= static_cast<uint8_t>(name[i]);
    hash *= 16777619UL;
  }

  return hash;
}

static uint32_t buildAttackSeed(
    int round,
    int attackerWeaponSlot,
    int defenderWeaponSlot,
    const String &attackerName,
    uint32_t salt
) {
  return
      hashPilotName(attackerName) +
      (round * 31UL) +
      (attackerWeaponSlot * 17UL) +
      (defenderWeaponSlot * 13UL) +
      salt;
}

static int rollDeterministic2D6(
    int round,
    int attackerWeaponSlot,
    int defenderWeaponSlot,
    const String &attackerName
) {
  uint32_t seedOne = buildAttackSeed(
      round,
      attackerWeaponSlot,
      defenderWeaponSlot,
      attackerName,
      23UL
  );

  uint32_t seedTwo = buildAttackSeed(
      round,
      attackerWeaponSlot,
      defenderWeaponSlot,
      attackerName,
      71UL
  );

  int dieOne = (seedOne % 6) + 1;
  int dieTwo = (seedTwo % 6) + 1;

  return dieOne + dieTwo;
}


static String resolveHitLocation(
    int round,
    int attackerWeaponSlot,
    int defenderWeaponSlot,
    const String &attackerName
) {
  static const char *hitTable[] = {
    "HEAD",
    "CT", "CT", "CT", "CT",
    "LT", "LT",
    "RT", "RT",
    "LA", "LA",
    "RA", "RA",
    "LL", "LL",
    "RL", "RL"
  };

  const int hitTableSize =
      sizeof(hitTable) / sizeof(hitTable[0]);

  uint32_t seed = buildAttackSeed(
      round,
      attackerWeaponSlot,
      defenderWeaponSlot,
      attackerName,
      113UL
  );

  int index = seed % hitTableSize;

  return String(hitTable[index]);
}

static bool weaponHasAmmo(
    const ActiveWeapon &activeWeapon
) {
  // Negative maxAmmo means the weapon does not use ammunition.
  if (activeWeapon.maxAmmo < 0) {
    return true;
  }

  return activeWeapon.currentAmmo > 0;
}

static void consumeWeaponAmmo(
    ActiveWeapon &activeWeapon
) {
  // Energy weapons and other unlimited weapons do nothing here.
  if (activeWeapon.maxAmmo < 0) {
    return;
  }

  if (activeWeapon.currentAmmo > 0) {
    activeWeapon.currentAmmo--;
  }
}

static void applyWeaponHeat(
    ActiveMech &mech,
    const WeaponProfile &weapon
) {
  mech.currentHeat += weapon.heat;
}

void resolveBattleTurn() {
  if (!battleTurnReady()) {
    return;
  }

  if (localBattleMech == nullptr ||
      remoteBattleMech == nullptr) {
    Serial.println(
        "Battle resolve failed: mech pointers are null"
    );
    return;
  }

  int localWeaponSlot = currentTurn.localWeaponSlot;
  int remoteWeaponSlot = currentTurn.remoteWeaponSlot;

  lastBattleResult = BattleRoundResult();
  lastBattleResult.round = currentTurn.round;

  bool localWeaponValid =
      localWeaponSlot >= 0 &&
      localWeaponSlot <
          static_cast<int>(
              localBattleMech->weapons.size()
          ) &&
      weaponHasAmmo(
          localBattleMech->weapons[localWeaponSlot]
      );

  bool remoteWeaponValid =
      remoteWeaponSlot >= 0 &&
      remoteWeaponSlot <
          static_cast<int>(
              remoteBattleMech->weapons.size()
          );

  /*
   * Your local attack against the remote mech.
   */

  if (localWeaponValid) {
    ActiveWeapon &localActiveWeapon =
      localBattleMech->weapons[localWeaponSlot];

    const WeaponProfile &localWeapon =
        localBattleMech
            ->weapons[localWeaponSlot]
            .weapon;
    
    consumeWeaponAmmo(localActiveWeapon);
    applyWeaponHeat(*localBattleMech, localWeapon);

    lastBattleResult.localHeatAdded =
        localWeapon.heat;

    lastBattleResult.localAmmoRemaining =
        localActiveWeapon.currentAmmo;

    const String &attackerName =
        localBattleMech->pilot.pilotName;

    int roll = rollDeterministic2D6(
        currentTurn.round,
        localWeaponSlot,
        remoteWeaponSlot,
        attackerName
    );

    int target = calculateTargetNumber();
    bool hit = roll >= target;

    lastBattleResult.localWeaponName =
        localWeapon.displayName;

    lastBattleResult.localAttackRoll = roll;
    lastBattleResult.localTargetNumber = target;
    lastBattleResult.localHit = hit;

    if (hit) {
      String locationId = resolveHitLocation(
          currentTurn.round,
          localWeaponSlot,
          remoteWeaponSlot,
          attackerName
      );

      ActiveLocation *location =
          findLocation(
              remoteBattleMech,
              locationId
          );

      if (location != nullptr) {
        applyDamageToLocation(
            *location,
            localWeapon.damage
        );

        lastBattleResult.localHitLocation =
            locationId;

        lastBattleResult.localDamageDealt =
            localWeapon.damage;

        Serial.print("Your attack rolled ");
        Serial.print(roll);
        Serial.print(" vs ");
        Serial.print(target);
        Serial.print(": HIT enemy ");
        Serial.print(locationId);
        Serial.print(" for ");
        Serial.println(localWeapon.damage);
      }
    } else {
      lastBattleResult.localHitLocation = "MISS";
      lastBattleResult.localDamageDealt = 0;

      Serial.print("Your attack rolled ");
      Serial.print(roll);
      Serial.print(" vs ");
      Serial.print(target);
      Serial.println(": MISS");
    }
  }

  /*
   * The remote attack against your local mech.
   */
  if (remoteWeaponValid) {
    ActiveWeapon &remoteActiveWeapon =
      remoteBattleMech->weapons[remoteWeaponSlot];
    const WeaponProfile &remoteWeapon =
        remoteBattleMech
            ->weapons[remoteWeaponSlot]
            .weapon;
    
    consumeWeaponAmmo(remoteActiveWeapon);
    applyWeaponHeat(*remoteBattleMech, remoteWeapon);

    lastBattleResult.remoteHeatAdded =
        remoteWeapon.heat;

    lastBattleResult.remoteAmmoRemaining =
        remoteActiveWeapon.currentAmmo;

    const String &attackerName =
        remoteBattleMech->pilot.pilotName;

    int roll = rollDeterministic2D6(
        currentTurn.round,
        remoteWeaponSlot,
        localWeaponSlot,
        attackerName
    );

    int target = calculateTargetNumber();
    bool hit = roll >= target;

    lastBattleResult.remoteWeaponName =
        remoteWeapon.displayName;

    lastBattleResult.remoteAttackRoll = roll;
    lastBattleResult.remoteTargetNumber = target;
    lastBattleResult.remoteHit = hit;

    if (hit) {
      String locationId = resolveHitLocation(
          currentTurn.round,
          remoteWeaponSlot,
          localWeaponSlot,
          attackerName
      );

      ActiveLocation *location =
          findLocation(
              localBattleMech,
              locationId
          );

      if (location != nullptr) {
        applyDamageToLocation(
            *location,
            remoteWeapon.damage
        );

        lastBattleResult.remoteHitLocation =
            locationId;

        lastBattleResult.remoteDamageDealt =
            remoteWeapon.damage;

        Serial.print("Enemy attack rolled ");
        Serial.print(roll);
        Serial.print(" vs ");
        Serial.print(target);
        Serial.print(": HIT your ");
        Serial.print(locationId);
        Serial.print(" for ");
        Serial.println(remoteWeapon.damage);
      }
    } else {
      lastBattleResult.remoteHitLocation = "MISS";
      lastBattleResult.remoteDamageDealt = 0;

      Serial.print("Enemy attack rolled ");
      Serial.print(roll);
      Serial.print(" vs ");
      Serial.print(target);
      Serial.println(": MISS");
    }
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

const BattleRoundResult &getLastBattleResult() {
  return lastBattleResult;
}

static bool isMechDestroyed(const ActiveMech *mech) {
  if (mech == nullptr) {
    return false;
  }

  for (const ActiveLocation &location : mech->locations) {
    if ((location.id == "HEAD" || location.id == "CT") &&
        location.currentStructure <= 0) {
      return true;
    }
  }

  return false;
}

bool isLocalMechDestroyed() {
  return isMechDestroyed(localBattleMech);
}

bool isRemoteMechDestroyed() {
  return isMechDestroyed(remoteBattleMech);
}