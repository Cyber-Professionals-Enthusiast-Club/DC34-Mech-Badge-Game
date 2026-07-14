#pragma once

#include "ActiveMech.h"

struct BattleTurn {
  int round = 1;
  int localWeaponSlot = -1;
  int remoteWeaponSlot = -1;
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

void submitLocalTurn(int weaponSlot);
void submitRemoteTurn(int weaponSlot);

bool battleTurnReady();
void resolveBattleTurn();

bool isLocalMechDestroyed();
bool isRemoteMechDestroyed();

const BattleTurn &getBattleTurn();
int getBattleRound();
const BattleRoundResult &getLastBattleResult();