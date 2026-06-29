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

//ENUMS
enum GameState {
  STATE_MAIN_MENU,
  STATE_STATUS,
  STATE_BATTLE_TEST,
  STATE_PILOT_RECORD,
  STATE_OPTIONS,
  STATE_CREDITS
};

//GLOBALS
GameState currentState = STATE_MAIN_MENU;
PilotProfile pilot;
ActiveMech playerMech;
DummyMechTarget dummyTarget;

int selectedMenuIndex = 0;
int selectedWeaponIndex = 0;
int selectedOptionsIndex = 0;

// bool inBattleTest = false;
// bool inOptionsMenu = false;
// bool inCreditsScreen = false;
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

//========FUNCTION PROTOTYPES
void handleMainMenu();
void handleOptionsMenu();
void handleCreditsScreen();
void handleBattleTest();
void returnToMainMenu();

//------Helpers
void returnToMainMenu() {
  //inBattleTest = false;
  //inOptionsMenu = false;
  //inCreditsScreen = false;
  matchWon = false;
  currentState = STATE_MAIN_MENU;
  drawMainMenu(selectedMenuIndex);
}
//============SETUP()
void setup() {
  Serial.begin(115200);
  delay(1500);

  randomSeed(esp_random());

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
  badgeSetup();
  drawMainMenu(selectedMenuIndex);
}


//===========LOOP()
void loop() {
  // keep existing matchWon handler here for now
  if (matchWon) {
  return;
  }

  if (currentState == STATE_BATTLE_TEST) {
    handleBattleTest();
    return;
  }

  if (currentState == STATE_CREDITS) {
    handleCreditsScreen();
    return;
  }

  if (currentState == STATE_OPTIONS) {
    handleOptionsMenu();
    return;
  }

  handleMainMenu();
}

void handleBattleTest() {
  if (digitalRead(BTN_B) == LOW) {
    returnToMainMenu();
    delay(180);
  }
}

void handleCreditsScreen() {
  if (digitalRead(BTN_B) == LOW) {
    currentState = STATE_OPTIONS;
    drawOptionsMenu(selectedOptionsIndex);
    delay(180);
  }
}
void handleOptionsMenu() {
  if (digitalRead(BTN_A) == LOW) {
    if (selectedOptionsIndex == 0) {
    currentState = STATE_CREDITS;
    drawCreditsScreen();
    }

    delay(180);
  }

if (digitalRead(BTN_B) == LOW) {
  returnToMainMenu();
  delay(180);
  }
}
void handleMainMenu() {
  if (digitalRead(BTN_UP) == LOW) {
    selectedMenuIndex--;
    if (selectedMenuIndex < 0) selectedMenuIndex = 3;
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
      currentState = STATE_STATUS;
      drawCockpitStatusTFT(playerMech);
    }

    if (selectedMenuIndex == 1) {
      // inBattleTest = true;
      currentState = STATE_BATTLE_TEST;
      drawBattleTestScreen(playerMech, dummyTarget, battleMessage, selectedWeaponIndex);
    }

    if (selectedMenuIndex == 2) {
      currentState = STATE_PILOT_RECORD;
      drawPilotRecordScreen(pilot);
    }

    if (selectedMenuIndex == 3) {
      selectedOptionsIndex = 0;
      currentState = STATE_OPTIONS;
      drawOptionsMenu(selectedOptionsIndex);
    }
    delay(180);
  }

  if (digitalRead(BTN_B) == LOW) {
    returnToMainMenu();
    delay(180);
  }
}
