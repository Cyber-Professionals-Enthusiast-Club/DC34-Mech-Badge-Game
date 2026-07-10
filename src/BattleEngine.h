#pragma once

#include "ActiveMech.h"

struct BattleTurn {
  int round = 1;
  int localWeaponSlot = -1;
  int remoteWeaponSlot = -1;
  bool localSubmitted = false;
  bool remoteSubmitted = false;
};

void startMultiplayerBattle(
    ActiveMech &localMech,
    ActiveMech &remoteMech
);

void submitLocalTurn(int weaponSlot);
void submitRemoteTurn(int weaponSlot);

bool battleTurnReady();
void resolveBattleTurn();

const BattleTurn &getBattleTurn();
int getBattleRound();