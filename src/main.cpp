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
  STATE_CREDITS,
  STATE_WIN_SCREEN,
  STATE_PLAYER_NAME,
  STATE_PLAYER_FACTION,
};

//GLOBALS
GameState currentState = STATE_MAIN_MENU;
PilotProfile pilot;
ActiveMech playerMech;
DummyMechTarget dummyTarget;

String newPilotName = "ACE";
int selectedNameChar = 0;
int selectedFactionIndex = 0;

const char* FACTIONS[] = {
  "BoomCorp LLC",
  "MeatSlab Inc.",
  "Reptile Sports Formula",
  "Happy Sushi LTD.",
  "Yuri's Consumables Inc",
  "SmoCorp Industrial Concern"
};

const int FACTION_COUNT = 6;

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
void handleWinScreen();
void handleStatusScreen();
void handlePilotRecordScreen();
void drawPlayerNameScreen();
void drawPlayerFactionScreen();
void handlePlayerNameScreen();
void handlePlayerFactionScreen();

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
  
  //TEST THIS-----------------------
  // LittleFS.remove("/pilot_profile.json");

  BadgeConfig badge;
  std::vector<ChassisProfile> chassisProfiles;
  std::vector<WeaponProfile> weaponProfiles;

  initDummyTarget(dummyTarget);

    bool pilotLoaded = loadPilotProfile(pilot);

    if (!pilotLoaded) {
      Serial.println("No pilot profile found; launching player creation.");
      pilot = PilotProfile();   // reset to defaults
    }

    if (pilot.pilotName == "") {
      badgeSetup();
      currentState = STATE_PLAYER_NAME;
      drawPlayerNameScreen();
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

   switch (currentState) {
    case STATE_WIN_SCREEN:
      handleWinScreen();
      break;

    case STATE_MAIN_MENU:
      handleMainMenu();
      break;

    case STATE_BATTLE_TEST:
      handleBattleTest();
      break;

    case STATE_OPTIONS:
      handleOptionsMenu();
      break;

    case STATE_CREDITS:
      handleCreditsScreen();
      break;

    case STATE_STATUS:
      handleStatusScreen();
      break;

    case STATE_PILOT_RECORD:
      handlePilotRecordScreen();
      break;

    case STATE_PLAYER_NAME:
      handlePlayerNameScreen();
      break;

    case STATE_PLAYER_FACTION:
      handlePlayerFactionScreen();
      break;
  }
}

//================HANDLER FUNCTIONS

void handleStatusScreen() {
  if (digitalRead(BTN_B) == LOW) {
    returnToMainMenu();
    delay(180);
  }
}

void handlePilotRecordScreen() {
  if (digitalRead(BTN_B) == LOW) {
    returnToMainMenu();
    delay(180);
  }
}

void handleWinScreen() {
  if (digitalRead(BTN_B) == LOW) {
    battleMessage = "Awaiting command...";
    selectedWeaponIndex = 0;

    initDummyTarget(dummyTarget);

    playerMech.currentHeat = 0;
    playerMech.shutdown = false;
    playerMech.shutdownTurnsRemaining = 0;

    for (ActiveWeapon &weapon : playerMech.weapons) {
      if (weapon.maxAmmo >= 0) {
        weapon.currentAmmo = weapon.maxAmmo;
      }
    }

    returnToMainMenu();
    delay(180);
  }
}
void handleBattleTest() {
  if (digitalRead(BTN_UP) == LOW) {
    selectedWeaponIndex--;

    if (selectedWeaponIndex < 0) {
      selectedWeaponIndex = playerMech.weapons.size(); 
      // +1 because index 0 is HOLD FIRE
    }

    drawBattleTestScreen(playerMech, dummyTarget, battleMessage, selectedWeaponIndex);
    delay(180);
  }

  if (digitalRead(BTN_DOWN) == LOW) {
    selectedWeaponIndex++;

    if (selectedWeaponIndex > playerMech.weapons.size()) {
      selectedWeaponIndex = 0;
    }

    drawBattleTestScreen(playerMech, dummyTarget, battleMessage, selectedWeaponIndex);
    delay(180);
  }

  if (digitalRead(BTN_A) == LOW) {
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
    pilot.wins++;
    pilot.kills++;
    pilot.battles++;
    pilot.xp += 100;

    savePilotProfile(pilot);

    currentState = STATE_WIN_SCREEN;
    drawWinScreen(dummyTarget.destroyedReason);

    delay(180);
    return;
  }

    if (dummyTarget.destroyed) {
      pilot.wins++;
      pilot.kills++;
      pilot.battles++;
      pilot.xp += 100;
      savePilotProfile(pilot);

      currentState = STATE_WIN_SCREEN;
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

void drawPlayerNameScreen() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_YELLOW);
  tft.setTextSize(2);
  tft.setCursor(20, 20);
  tft.println("NEW PILOT");

  tft.setTextColor(ST77XX_GREEN);
  tft.setCursor(20, 80);
  tft.print("CALLSIGN: ");
  tft.println(newPilotName);

  tft.setTextSize(1);
  tft.setCursor(20, 205);
  tft.println("UP=ADD A  DOWN=DELETE");
  tft.setCursor(20, 220);
  tft.println("A=NEXT");
}

void handlePlayerNameScreen() {
  if (digitalRead(BTN_UP) == LOW) {
    if (newPilotName.length() < 10) {
      newPilotName += "A";
    }
    drawPlayerNameScreen();
    delay(180);
  }

  if (digitalRead(BTN_DOWN) == LOW) {
    if (newPilotName.length() > 0) {
      newPilotName.remove(newPilotName.length() - 1);
    }
    drawPlayerNameScreen();
    delay(180);
  }

  if (digitalRead(BTN_A) == LOW) {
    currentState = STATE_PLAYER_FACTION;
    drawPlayerFactionScreen();
    delay(180);
  }
}
void drawPlayerFactionScreen() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_YELLOW);
  tft.setTextSize(2);
  tft.setCursor(20, 15);
  tft.println("FACTION");

  tft.setTextSize(1);

  for (int i = 0; i < FACTION_COUNT; i++) {
    int y = 50 + (i * 22);
    tft.setCursor(20, y);

    if (i == selectedFactionIndex) {
      tft.setTextColor(ST77XX_YELLOW);
      tft.print("> ");
    } else {
      tft.setTextColor(ST77XX_GREEN);
      tft.print("  ");
    }

    tft.println(FACTIONS[i]);
  }

  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(20, 220);
  tft.println("A=CONFIRM  B=BACK");
} 

void handlePlayerFactionScreen() {
  if (digitalRead(BTN_UP) == LOW) {
    selectedFactionIndex--;
    if (selectedFactionIndex < 0) selectedFactionIndex = FACTION_COUNT - 1;
    drawPlayerFactionScreen();
    delay(180);
  }

  if (digitalRead(BTN_DOWN) == LOW) {
    selectedFactionIndex++;
    if (selectedFactionIndex >= FACTION_COUNT) selectedFactionIndex = 0;
    drawPlayerFactionScreen();
    delay(180);
  }

  if (digitalRead(BTN_B) == LOW) {
    currentState = STATE_PLAYER_NAME;
    drawPlayerNameScreen();
    delay(180);
  }

  if (digitalRead(BTN_A) == LOW) {
    pilot.pilotName = newPilotName;
    pilot.faction = FACTIONS[selectedFactionIndex];
    pilot.xp = 0;
    pilot.level = 1;
    pilot.skillPoints = 0;
    pilot.wins = 0;
    pilot.losses = 0;
    pilot.kills = 0;
    pilot.deaths = 0;
    pilot.battles = 0;

    savePilotProfile(pilot);

    currentState = STATE_MAIN_MENU;
    drawMainMenu(selectedMenuIndex);
    delay(180);
  }
}