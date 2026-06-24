#pragma once

#include <Arduino.h>
#include <vector>
#include "ActiveMech.h"

struct DummyLocation {
  String id;
  int maxArmor;
  int currentArmor;
  int maxStructure;
  int currentStructure;
};

struct DummyMechTarget {
  String name = "TARGET DUMMY";
  std::vector<DummyLocation> locations;
  bool destroyed = false;
  String destroyedReason = "";
};

void initDummyTarget(DummyMechTarget &target);
void drawBattleTestScreen(const ActiveMech &mech, const DummyMechTarget &target, const String &message, int selectedWeaponIndex);
void attackDummyMech(ActiveMech &mech, DummyMechTarget &target, String &message, int selectedWeaponIndex);
void holdFire(ActiveMech &mech, String &message);