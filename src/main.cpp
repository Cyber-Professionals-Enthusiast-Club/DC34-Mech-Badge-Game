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
#include "PlayerBuilderUI.h"
#include "GameState.h"
#include "BleDuel.h"
#include "BLEComms.h"
#include "RadarUI.h"
#include "WirelessProtocol.h"

//ENUMS


//GLOBALS
GameState currentState = STATE_MAIN_MENU;
PilotProfile pilot;
ActiveMech playerMech;
DummyMechTarget dummyTarget;

bool radarTargetLocked = false;

String newPilotName = "ACE";
int selectedNameChar = 0;

int selectedRadarIndex = 0;


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

bool inBattleTest = false;
// bool inOptionsMenu = false;
// bool inCreditsScreen = false;
bool matchWon = false;

char nameBuffer[9] = "        ";
int nameCursor;
int selectedFactionIndex;

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
void handlePlayerNameScreen();
void handlePlayerFactionScreen();
void handleRadarScreen();
void handleChallengeSentScreen();

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

    CpecAdvertisedPilot advertisedPilot;
    advertisedPilot.pilotName = pilot.pilotName;
    advertisedPilot.chassisId = chassisCodeFromId(badge.chassisId);
    advertisedPilot.factionId = 0; // temporary

    bleSetup(advertisedPilot);

  badgeSetup();
  // bleDuelSetup();
  drawMainMenu(selectedMenuIndex);
}


//===========LOOP()
void loop() {

bleLoop();

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

    case STATE_RADAR:
      handleRadarScreen();
      break;

    case STATE_CHALLENGE_SENT:
      handleChallengeSentScreen();
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

void handleRadarScreen() {
    if (radarTargetLocked) {
    if (digitalRead(BTN_B) == LOW) {
      radarTargetLocked = false;
      drawRadarScreen(selectedRadarIndex);
      delay(180);
    }
    return;
  }

  int count = getNearbyBadgeCount();

  if (selectedRadarIndex >= count && count > 0) {
    selectedRadarIndex = count - 1;
  }

  if (selectedRadarIndex < 0) {
    selectedRadarIndex = 0;
  }

  if (digitalRead(BTN_UP) == LOW && count > 0) {
    selectedRadarIndex--;
    if (selectedRadarIndex < 0) selectedRadarIndex = count - 1;

    drawRadarScreen(selectedRadarIndex);
    delay(180);
    return;
  }

  if (digitalRead(BTN_DOWN) == LOW && count > 0) {
    selectedRadarIndex++;
    if (selectedRadarIndex >= count) selectedRadarIndex = 0;

    drawRadarScreen(selectedRadarIndex);
    delay(180);
    return;
  }

  if (digitalRead(BTN_A) == LOW && count > 0) {
    NearbyBadge badge = getNearbyBadge(selectedRadarIndex);

    currentState = STATE_CHALLENGE_SENT;
      delay(180);
      return;

    delay(180);
    return;
  }

  if (digitalRead(BTN_B) == LOW) {
    currentState = STATE_MAIN_MENU;
    drawMainMenu(selectedMenuIndex);
    delay(180);
    return;
  }

  static unsigned long lastRadarDrawMs = 0;

  if (millis() - lastRadarDrawMs > 1000) {
    lastRadarDrawMs = millis();
    drawRadarScreen(selectedRadarIndex);
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
  if (selectedMenuIndex == 0) {
    currentState = STATE_STATUS;
    drawCockpitStatusTFT(playerMech);
  }

  if (selectedMenuIndex == 1) {
    currentState = STATE_BATTLE_TEST;
    drawBattleTestScreen(playerMech, dummyTarget, battleMessage, selectedWeaponIndex);
  }

  if (selectedMenuIndex == 2) {
    currentState = STATE_RADAR;
    drawRadarScreen(selectedRadarIndex);
  }

  if (selectedMenuIndex == 3) {
    currentState = STATE_PILOT_RECORD;
    drawPilotRecordScreen(pilot);
  }

  if (selectedMenuIndex == 4) {
    currentState = STATE_OPTIONS;
    selectedOptionsIndex = 0;
    drawOptionsMenu(selectedOptionsIndex);
  }

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
    if (selectedMenuIndex < 0) selectedMenuIndex = 4;
    drawMainMenu(selectedMenuIndex);
    delay(180);
  }

  if (digitalRead(BTN_DOWN) == LOW) {
    selectedMenuIndex++;
    if (selectedMenuIndex > 4) selectedMenuIndex = 0;
    drawMainMenu(selectedMenuIndex);
    delay(180);
  }

  if (digitalRead(BTN_A) == LOW) {
    if (selectedMenuIndex == 0) {
      currentState = STATE_STATUS;
      drawCockpitStatusTFT(playerMech);
    }

    if (selectedMenuIndex == 1) {
      currentState = STATE_BATTLE_TEST;
      drawBattleTestScreen(playerMech, dummyTarget, battleMessage, selectedWeaponIndex);
    }

    if (selectedMenuIndex == 2) {
      currentState = STATE_RADAR;

      tft.fillScreen(ST77XX_BLACK);
      tft.setTextColor(ST77XX_GREEN);
      tft.setTextSize(2);
      tft.setCursor(20, 20);
      tft.println("RADAR");

      tft.setTextSize(1);
      tft.setCursor(20, 60);
      tft.println("Area scan initializing...");

      tft.setCursor(20, 220);
      tft.println("B=BACK");
    }

    if (selectedMenuIndex == 3) {
      currentState = STATE_PILOT_RECORD;
      drawPilotRecordScreen(pilot);
    }

    if (selectedMenuIndex == 4) {
      currentState = STATE_OPTIONS;
      selectedOptionsIndex = 0;
      drawOptionsMenu(selectedOptionsIndex);
    }

    delay(180);
  }
}

void handlePlayerNameScreen() {
    if (digitalRead(BTN_UP) == LOW) {
      if (nameBuffer[nameCursor] < 'A' || nameBuffer[nameCursor] > 'Z') {
        nameBuffer[nameCursor] = 'A';
      } else {
        nameBuffer[nameCursor]++;

        if (nameBuffer[nameCursor] > 'Z') {
          nameBuffer[nameCursor] = 'A';
        }
      }

      drawPlayerNameScreen();
      delay(180);
    }

    if (digitalRead(BTN_DOWN) == LOW) {
      if (nameBuffer[nameCursor] < 'A' || nameBuffer[nameCursor] > 'Z') {
        nameBuffer[nameCursor] = 'Z';
      } else {
        nameBuffer[nameCursor]--;

        if (nameBuffer[nameCursor] < 'A') {
          nameBuffer[nameCursor] = 'Z';
        }
      }

      drawPlayerNameScreen();
      delay(180);
    }

  if (digitalRead(BTN_LEFT) == LOW) {
    nameCursor--;

    if (nameCursor < 0) {
      nameCursor = 7;
    }

    drawPlayerNameScreen();
    delay(180);
  }

  if (digitalRead(BTN_RIGHT) == LOW) {
    nameCursor++;

    if (nameCursor > 7) {
      nameCursor = 0;
    }

    drawPlayerNameScreen();
    delay(180);
  }

  if (digitalRead(BTN_A) == LOW) {
    newPilotName = String(nameBuffer);

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

void handleChallengeSentScreen() {

  tft.fillScreen(ST77XX_BLACK);

  tft.setTextColor(ST77XX_GREEN);

  tft.setTextSize(2);
  tft.setCursor(20,20);
  tft.println("CHALLENGE");

  tft.setCursor(20,50);
  tft.println("SENT");

  tft.setTextSize(1);

  tft.setCursor(20,100);
  tft.println("Waiting...");

  tft.setCursor(20,220);
  tft.println("B = Cancel");

  if (digitalRead(BTN_B) == LOW) {

      currentState = STATE_RADAR;
      drawRadarScreen(selectedRadarIndex);
      delay(180);
  }
}