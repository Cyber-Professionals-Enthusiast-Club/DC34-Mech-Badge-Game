#pragma once

#include "ActiveMech.h"

enum MovementChoice {
  MOVE_CLOSE = -1,
  MOVE_HOLD = 0,
  MOVE_DISTANCE = 1
};

enum BattleRange {
  RANGE_MELEE = 0,
  RANGE_CLOSE = 1,
  RANGE_MEDIUM = 2,
  RANGE_LONG = 3
};

struct BattleTurn {
  int round = 1;
  int localWeaponSlot = -1;
  int remoteWeaponSlot = -1;
  MovementChoice localMovement = MOVE_HOLD;
  MovementChoice remoteMovement = MOVE_HOLD;
  bool localSubmitted = false;
  bool remoteSubmitted = false;
};

struct BattleRoundResult {
  int round = 0;

  String localWeaponName;
  String remoteWeaponName;

  bool localHit = false;
  bool remoteHit = false;

  String localHitLocation;
  String remoteHitLocation;

  int localAttackRoll = 0;
  int remoteAttackRoll = 0;

  int localTargetNumber = 0;
  int remoteTargetNumber = 0;

  int localDamageDealt = 0;
  int remoteDamageDealt = 0;

  int localHeatAdded = 0;
  int remoteHeatAdded = 0;

  int localAmmoRemaining = -1;
  int remoteAmmoRemaining = -1;
};

void startMultiplayerBattle(
    ActiveMech &localMech,
    ActiveMech &remoteMech
);

void submitLocalTurn(
    int weaponSlot,
    MovementChoice movement
);

void submitRemoteTurn(
    int weaponSlot,
    MovementChoice movement
);

bool battleTurnReady();
void resolveBattleTurn();

bool isLocalMechDestroyed();
bool isRemoteMechDestroyed();

const BattleTurn &getBattleTurn();
int getBattleRound();
const BattleRoundResult &getLastBattleResult();

BattleRange getBattleRange();
String battleRangeName(BattleRange range);