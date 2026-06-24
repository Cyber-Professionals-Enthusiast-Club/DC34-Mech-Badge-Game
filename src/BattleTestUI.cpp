#include "BattleTestUI.h"
#include "CPEC_BattleMech_TextPort.h"

#define COLOR_GREY 0x8410

static uint16_t getLocationColor(const DummyLocation &loc) {
  int current = loc.currentArmor + loc.currentStructure;
  int maximum = loc.maxArmor + loc.maxStructure;

  if (loc.currentStructure <= 0) {
    return COLOR_GREY;
  }

  float pct = (float)current / (float)maximum;

  if (pct <= 0.25f) {
    return ST77XX_RED;
  }

  if (pct <= 0.50f) {
    return ST77XX_YELLOW;
  }

  return ST77XX_GREEN;
}

uint16_t getLocationColor(
  int currentArmor,
  int currentStructure,
  int maxArmor,
  int maxStructure
) {
  int current = currentArmor + currentStructure;
  int maximum = maxArmor + maxStructure;

  float pct = (float)current / (float)maximum;

  if (currentStructure <= 0) {
    return ST77XX_WHITE;  // destroyed
  }

  if (pct <= 0.25f) {
    return ST77XX_RED;
  }

  if (pct <= 0.50f) {
    return ST77XX_YELLOW;
  }

  return ST77XX_GREEN;
}

static int rollLocationIndex() {
  return random(0, 8);
}

void initDummyTarget(DummyMechTarget &target) {
  target.name = "DUMMY MECH";
  target.destroyed = false;
  target.destroyedReason = "";

  target.locations.clear();

  target.locations.push_back({"HEAD", 3, 3, 1, 1});
  target.locations.push_back({"CT",   8, 8, 4, 4});
  target.locations.push_back({"LT",   6, 6, 3, 3});
  target.locations.push_back({"RT",   6, 6, 3, 3});
  target.locations.push_back({"LA",   4, 4, 2, 2});
  target.locations.push_back({"RA",   4, 4, 2, 2});
  target.locations.push_back({"LL",   5, 5, 2, 2});
  target.locations.push_back({"RL",   5, 5, 2, 2});
}

void drawBattleTestScreen(const ActiveMech &mech, const DummyMechTarget &target, const String &message,  int selectedWeaponIndex) {
  tft.fillScreen(ST77XX_BLACK);
  tft.fillRect(10, 180, 300, 25, ST77XX_BLACK);
  tft.setTextWrap(false);

  tft.setTextColor(ST77XX_RED);
  tft.setTextSize(2);
  tft.setCursor(20, 10);
  tft.println("BATTLE TEST");

  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(1);
  tft.setCursor(20, 38);
  tft.print("MECH: ");
  tft.println(mech.chassis.displayName);
if (mech.currentHeat < 5) {
  tft.setTextColor(ST77XX_GREEN);
}
else if (mech.currentHeat < 8) {
  tft.setTextColor(ST77XX_YELLOW);
}
else {
  tft.setTextColor(ST77XX_RED);
}

tft.setCursor(20, 62);
tft.print("HEAT: ");
tft.print(mech.currentHeat);
tft.print("/");
tft.println(mech.chassis.heatCapacity);
tft.setCursor(140, 70);

tft.setTextColor(ST77XX_GREEN);
tft.println("WEAPONS");
int weaponY = 85;
// Virtual option: HOLD FIRE
tft.setCursor(140, weaponY);

if (selectedWeaponIndex == 0) {
  tft.setTextColor(ST77XX_YELLOW);
  tft.print("> ");
} else {
  tft.setTextColor(ST77XX_GREEN);
  tft.print("  ");
}

tft.println("HOLD FIRE");
weaponY += 12;

// Real weapons start at selectedWeaponIndex 1
for (int i = 0; i < mech.weapons.size(); i++) {
  int menuIndex = i + 1;

  tft.setCursor(140, weaponY);

  if (selectedWeaponIndex == menuIndex) {
    tft.setTextColor(ST77XX_YELLOW);
    tft.print("> ");
  } else {
    tft.setTextColor(ST77XX_GREEN);
    tft.print("  ");
  }

    String weaponLine = mech.weapons[i].weapon.displayName;

    if (mech.weapons[i].currentAmmo >= 0) {
      weaponLine += " (";
      weaponLine += String(mech.weapons[i].currentAmmo);
      weaponLine += "/";
      weaponLine += String(mech.weapons[i].maxAmmo);
      weaponLine += ")";
    }

tft.println(weaponLine);
  weaponY += 12;
}

  tft.setCursor(20, 52);
  tft.print("TARGET: ");
  tft.println(target.name);

  int y = 75;
for (const DummyLocation &loc : target.locations) {
  tft.setTextColor(getLocationColor(loc));

  tft.setCursor(10, y);
  tft.print(loc.id);
  tft.print(" A:");
  tft.print(loc.currentArmor);
  tft.print("/");
  tft.print(loc.maxArmor);
  tft.print(" S:");
  tft.print(loc.currentStructure);
  tft.print("/");
  tft.println(loc.maxStructure);

  y += 12;
}

tft.setTextColor(ST77XX_GREEN);

tft.setCursor(10, 180);
tft.setTextColor(ST77XX_CYAN);
tft.println(message);

  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(20, 220);
  tft.println("A = ATTACK   B = BACK");
}

static DummyLocation* findLocation(DummyMechTarget &target, const String &id) {
  for (DummyLocation &loc : target.locations) {
    if (loc.id == id) return &loc;
  }
  return nullptr;
}

static String transferLocationFor(const String &id) {
  if (id == "LA") return "LT";
  if (id == "RA") return "RT";
  if (id == "LL") return "LT";
  if (id == "RL") return "RT";
  if (id == "LT") return "CT";
  if (id == "RT") return "CT";
  return "";
}

static void checkTargetDestroyed(DummyMechTarget &target) {
  DummyLocation *head = findLocation(target, "HEAD");
  DummyLocation *ct = findLocation(target, "CT");

  if (head && head->currentStructure <= 0) {
    target.destroyed = true;
    target.destroyedReason = "HEAD DESTROYED";
    return;
  }

  if (ct && ct->currentStructure <= 0) {
    target.destroyed = true;
    target.destroyedReason = "CT DESTROYED";
    return;
  }
}

static void applyDamageToLocation(
  DummyMechTarget &target,
  DummyLocation &loc,
  int damage
) {
  if (target.destroyed) return;

  if (loc.currentStructure <= 0) {
    String transferId = transferLocationFor(loc.id);
    DummyLocation *transferLoc = findLocation(target, transferId);

    if (transferLoc) {
      applyDamageToLocation(target, *transferLoc, damage);
    }

    return;
  }

  int remaining = damage;

  if (loc.currentArmor > 0) {
    int armorDamage = min(loc.currentArmor, remaining);
    loc.currentArmor -= armorDamage;
    remaining -= armorDamage;
  }

  if (remaining > 0 && loc.currentStructure > 0) {
    int structureDamage = min(loc.currentStructure, remaining);
    loc.currentStructure -= structureDamage;
    remaining -= structureDamage;
  }

  if (remaining > 0 && loc.currentStructure <= 0) {
    String transferId = transferLocationFor(loc.id);
    DummyLocation *transferLoc = findLocation(target, transferId);

    if (transferLoc) {
      applyDamageToLocation(target, *transferLoc, remaining);
    }
  }

  checkTargetDestroyed(target);
}

void attackDummyMech(
  ActiveMech &mech,
  DummyMechTarget &target,
  String &message,
  int selectedWeaponIndex
) {
  if (mech.weapons.size() == 0) {
    message = "No weapons!";
    return;
  }

  if (target.locations.size() == 0) {
    initDummyTarget(target);
  }

  ActiveWeapon &weapon =
  mech.weapons[selectedWeaponIndex];

    if (weapon.currentAmmo == 0) {
      message = "OUT OF AMMO";
      return;
    }

    if (weapon.currentAmmo > 0) {
      weapon.currentAmmo--;
      
    }
      mech.currentHeat += weapon.weapon.heat;

    if (mech.currentHeat >= mech.chassis.heatCapacity) {
      mech.currentHeat = mech.chassis.heatCapacity;
      mech.shutdown = true;
      mech.shutdownTurnsRemaining = 2;
      message = "OVERHEAT! SHUTDOWN 2 TURNS";
    return;
  }

  int die1 = random(1, 7);
  int die2 = random(1, 7);
  int roll = die1 + die2;

  int baseTarget = 7;
  int needed = baseTarget - weapon.weapon.accuracyModifier;

  if (roll < needed) {
    message = weapon.weapon.displayName;
    message += " MISS ";
    message += String(roll);
    message += "/";
    message += String(needed);
    return;
  }

  int damage = weapon.weapon.damage;

  int locIndex = rollLocationIndex();
  DummyLocation &loc = target.locations[locIndex];

  applyDamageToLocation(target, loc, damage);

  message = weapon.weapon.displayName;
  message += " HIT ";
  message += String(roll);
  message += "/";
  message += String(needed);
  message += " ";
  message += loc.id;
  message += " DMG ";
  message += String(damage);

  if (loc.currentStructure == 0) {
    message += " ==DESTROYED==";
  }

if (mech.shutdown) {
  tft.setTextColor(ST77XX_RED);
  tft.setCursor(20, 64);
  tft.print("SHUTDOWN ");
  tft.print(mech.shutdownTurnsRemaining);
  tft.println(" TURNS");
}
}

void holdFire(
  ActiveMech &mech,
  String &message
) {
  const int coolAmount = 3;

  if (mech.shutdown) {
    mech.shutdownTurnsRemaining--;

    mech.currentHeat -= coolAmount;
    if (mech.currentHeat < 0) {
      mech.currentHeat = 0;
    }

    if (mech.shutdownTurnsRemaining <= 0) {
      mech.shutdown = false;

      if (mech.currentHeat > mech.chassis.heatCapacity - 3) {
        mech.currentHeat = mech.chassis.heatCapacity - 3;
      }

      message = "REACTOR ONLINE HEAT ";
      message += String(mech.currentHeat);
      return;
    }

    message = "REBOOTING ";
    message += String(mech.shutdownTurnsRemaining);
    message += " TURNS";
    return;
  }

  int before = mech.currentHeat;

  mech.currentHeat -= coolAmount;
  if (mech.currentHeat < 0) {
    mech.currentHeat = 0;
  }

  message = "HOLD FIRE HEAT ";
  message += String(before);
  message += "->";
  message += String(mech.currentHeat);
}