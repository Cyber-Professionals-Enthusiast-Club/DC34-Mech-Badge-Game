#include <Arduino.h>
#include <LittleFS.h>
#include "ProfileLoader.h"
#include "ActiveMech.h"
#include "CockpitUI.h"
#include "CPEC_BattleMech_TextPort.h"
#include "MenuUI.h"
#include "BattleTestUI.h"
#include "OptionsUI.h"
#include "PilotRecordUI.h"

enum GameState {
  STATE_MAIN_MENU,
  STATE_STATUS,
  STATE_BATTLE_TEST,
  STATE_PILOT_RECORD,
  STATE_OPTIONS,
  STATE_CREDITS
};

GameState currentState = STATE_MAIN_MENU;
PilotProfile pilot;
ActiveMech playerMech;
DummyMechTarget dummyTarget;

int selectedMenuIndex = 0;
int selectedWeaponIndex = 0;
int selectedOptionsIndex = 0;

bool inBattleTest = false;
bool inOptionsMenu = false;

bool matchWon = false;

String battleMessage = "Awaiting command...";

//============DRAW WIN SCREEN
void drawWinScreen(const String &reason) {
  tft.fillScreen(ST77XX_BLACK);

  tft.setTextColor(ST77XX_YELLOW);
  tft.setTextSize(3);
  tft.setCursor(70, 60);
  tft.println("YOU WON!");

  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(1);
  tft.setCursor(45, 110);
  tft.print("Target destroyed: ");
  tft.println(reason);

  tft.setCursor(55, 160);
  tft.println("Press B for Main Menu");
}
//============SETUP()
void setup() {
  Serial.begin(115200);
  delay(1500);

  randomSeed(esp_random());

  Serial.println("=== CPEC MECHFIGHTERS ===");

  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS mount failed");
    return;
  }

  BadgeConfig badge;
  std::vector<ChassisProfile> chassisProfiles;
  std::vector<WeaponProfile> weaponProfiles;

  initDummyTarget(dummyTarget);

  if (!loadPilotProfile(pilot)) {
    Serial.println("Pilot load failed");
    return;
  }

  if (!loadBadgeConfig(badge)) {
    Serial.println("Badge load failed");
    return;
  }

  if (!loadChassisProfiles(chassisProfiles)) {
    Serial.println("Chassis profiles failed");
    return;
  }

  if (!loadWeaponProfiles(weaponProfiles)) {
    Serial.println("Weapon profiles failed");
    return;
  }

  const ChassisProfile *activeChassis =
      findChassisById(chassisProfiles, badge.chassisId);

  if (!activeChassis) {
    Serial.print("No chassis found for ID: ");
    Serial.println(badge.chassisId);
    return;
  }

if (!buildActiveMech(playerMech, pilot, badge, activeChassis, weaponProfiles)) {
  Serial.println("ActiveMech build failed");
  return;
}

  if (!buildActiveMech(playerMech, pilot, badge, activeChassis, weaponProfiles)) {
    Serial.println("ActiveMech build failed");
    return;
  }

  Serial.print("Pilot: ");
  Serial.println(pilot.pilotName);

  Serial.print("Badge: ");
  Serial.println(badge.badgeId);

  Serial.print("Mech: ");
  Serial.println(activeChassis->displayName);


  Serial.println("Calling badgeSetup()");
  badgeSetup();
  Serial.println("Returned from badgeSetup()");

  // printActiveMech(playerMech);
  // drawCockpitStatusSerial(playerMech);
  // drawCockpitStatusTFT(playerMech);
  drawMainMenu(0);
}
//===========LOOP()
void loop() {
if (matchWon) {
  if (digitalRead(BTN_B) == LOW) {
    matchWon = false;
    inBattleTest = false;

    battleMessage = "Awaiting command...";
    selectedWeaponIndex = 0;

    // Rebuild/reset the dummy mech
    initDummyTarget(dummyTarget);

    // Reset player battle state too
    playerMech.currentHeat = 0;
    playerMech.shutdown = false;
    playerMech.shutdownTurnsRemaining = 0;

    // Refill ammo
    for (ActiveWeapon &weapon : playerMech.weapons) {
      if (weapon.maxAmmo >= 0) {
        weapon.currentAmmo = weapon.maxAmmo;
      }
    }

    drawMainMenu(selectedMenuIndex);
    delay(180);
  }

  return;
}


//BATTLE MENU CONTROLS
  if (inBattleTest) {
if (digitalRead(BTN_UP) == LOW) {

  int battleOptionCount =
      playerMech.weapons.size() + 1;

  selectedWeaponIndex--;

  if (selectedWeaponIndex < 0) {
    selectedWeaponIndex =
      battleOptionCount - 1;
  }

  drawBattleTestScreen(
      playerMech,
      dummyTarget,
      battleMessage,
      selectedWeaponIndex);

  delay(180);
}
if (digitalRead(BTN_DOWN) == LOW) {

  int battleOptionCount =
      playerMech.weapons.size() + 1;

  selectedWeaponIndex++;

  if (selectedWeaponIndex >= battleOptionCount) {
    selectedWeaponIndex = 0;
  }

  drawBattleTestScreen(
      playerMech,
      dummyTarget,
      battleMessage,
      selectedWeaponIndex);

  delay(180);
}

if (digitalRead(BTN_A) == LOW) {

  if (playerMech.shutdown &&
      selectedWeaponIndex != 0) {

    battleMessage = "SHUTDOWN - HOLD FIRE";

    drawBattleTestScreen(
      playerMech,
      dummyTarget,
      battleMessage,
      selectedWeaponIndex
    );

    delay(180);
    return;
  }

  if (selectedWeaponIndex == 0) {
    holdFire(playerMech, battleMessage);
  } else {

    int realWeaponIndex = selectedWeaponIndex - 1;

    attackDummyMech(
      playerMech,
      dummyTarget,
      battleMessage,
      realWeaponIndex
    );
    if (dummyTarget.destroyed) {
      matchWon = true;
      inBattleTest = false;
      drawWinScreen(dummyTarget.destroyedReason);
      delay(180);
      return;
  }
  }

  drawBattleTestScreen(
    playerMech,
    dummyTarget,
    battleMessage,
    selectedWeaponIndex
  );

  delay(180);
}

    if (digitalRead(BTN_B) == LOW) {
      inBattleTest = false;
      drawMainMenu(selectedMenuIndex);
      delay(180);
    }

    return;
  }

  //OPTIONS MENU
if (inOptionsMenu) {
  if (digitalRead(BTN_A) == LOW) {
    drawCreditsScreen();
    delay(180);
  }

  if (digitalRead(BTN_B) == LOW) {
    inOptionsMenu = false;
    drawMainMenu(selectedMenuIndex);
    delay(180);
  }

  return;
}
//MAIN MENU CONTROLS
  if (digitalRead(BTN_UP) == LOW) {
    selectedMenuIndex--;
    if (selectedMenuIndex < 0) selectedMenuIndex = 2;
    drawMainMenu(selectedMenuIndex);
    delay(180);
  }

  if (digitalRead(BTN_DOWN) == LOW) {
    selectedMenuIndex++;
    if (selectedMenuIndex > 3) selectedMenuIndex = 0;
    drawMainMenu(selectedMenuIndex);
    delay(180);
  }

  if (digitalRead(BTN_A) == LOW) {
    if (selectedMenuIndex == 0) {
      drawCockpitStatusTFT(playerMech);
    }

    if (selectedMenuIndex == 1) {
      inBattleTest = true;
      drawBattleTestScreen(playerMech, dummyTarget, battleMessage, selectedWeaponIndex);
    }

    if (selectedMenuIndex == 2) {
      drawPilotRecordScreen(pilot);
    }

    if (selectedMenuIndex == 3) {
      inOptionsMenu = true;
      selectedOptionsIndex = 0;
      drawOptionsMenu(selectedOptionsIndex);
    }

    delay(180);
  }

  if (digitalRead(BTN_B) == LOW) {
    drawMainMenu(selectedMenuIndex);
    delay(180);
  }
}