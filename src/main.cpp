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
  STATE_BOOT,
  STATE_PLAYER_BUILDER,
  STATE_MAIN_MENU,
  STATE_STATUS,
  STATE_BATTLE_TEST,
  STATE_PILOT_RECORD,
  STATE_OPTIONS,
  STATE_CREDITS,
  STATE_WIN_SCREEN
};

GameState currentState = STATE_BOOT;

bool inPlayerBuilder = false;
int builderStep = 0; // 0 = name, 1 = faction

String newPilotName = "ACE";
int nameCharIndex = 0;
int selectedFactionIndex = 0;

void drawNameBuilder();
void drawFactionBuilder();

const char* FACTIONS[] = {
  "BoomCorp LLC",
  "MeatSlab Inc.",
  "Reptile Sports Formula",
  "Happy Sushi LTD.",
  "Yuri's Consumables Inc",
  "SmoCorp Industrial Concern"
};

const int FACTION_COUNT = 6;

bool inOptionsMenu = false;
bool inCreditsScreen = false;
int selectedOptionsIndex = 0;

bool matchWon = false;

ActiveMech playerMech;
int selectedMenuIndex = 0;
int selectedWeaponIndex = 0;

DummyMechTarget dummyTarget;
String battleMessage = "Awaiting command...";
bool inBattleTest = false;

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

  PilotProfile pilot;

void changeState(GameState newState);

void changeState(GameState newState) {
  currentState = newState;

  switch (currentState) {
    case STATE_MAIN_MENU:
      drawMainMenu(selectedMenuIndex);
      break;

    case STATE_STATUS:
      drawCockpitStatusTFT(playerMech);
      break;

    case STATE_BATTLE_TEST:
      drawBattleTestScreen(playerMech, dummyTarget, battleMessage, selectedWeaponIndex);
      break;

    case STATE_PILOT_RECORD:
      drawPilotRecordScreen(pilot);
      break;

    case STATE_OPTIONS:
      drawOptionsMenu(selectedOptionsIndex);
      break;

    case STATE_CREDITS:
      drawCreditsScreen();
      break;

    case STATE_WIN_SCREEN:
      drawWinScreen(dummyTarget.destroyedReason);
      break;

    case STATE_PLAYER_BUILDER:
      builderStep = 0;
      drawNameBuilder();
      break;

    default:
      break;
  }
}

void setup() {
    Serial.begin(115200);
    delay(3000);

  badgeSetup();

drawMainMenu(selectedMenuIndex);
}

void drawNameBuilder() {
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
  tft.setCursor(20, 200);
  tft.println("UP/DOWN CHANGE  A=NEXT");
}


void drawFactionBuilder() {
  tft.fillScreen(ST77XX_BLACK);

  tft.setTextColor(ST77XX_YELLOW);
  tft.setTextSize(2);
  tft.setCursor(20, 15);
  tft.println("SELECT FACTION");

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

void loop() {
  if (inCreditsScreen) {
    if (digitalRead(BTN_B) == LOW) {
      inCreditsScreen = false;
      drawOptionsMenu(selectedOptionsIndex);
      delay(180);
    }
    return;
  }

  if (inOptionsMenu) {
    if (digitalRead(BTN_A) == LOW) {
      if (selectedOptionsIndex == 0) {
        inCreditsScreen = true;
        drawCreditsScreen();
      }
      delay(180);
    }

    if (digitalRead(BTN_B) == LOW) {
      inOptionsMenu = false;
      drawMainMenu(selectedMenuIndex);
      delay(180);
    }

    return;
  }

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
      drawCockpitStatusTFT(playerMech);
    }

    if (selectedMenuIndex == 1) {
      inBattleTest = true;

      drawBattleTestScreen(
        playerMech,
        dummyTarget,
        battleMessage,
        selectedWeaponIndex
      );
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
}