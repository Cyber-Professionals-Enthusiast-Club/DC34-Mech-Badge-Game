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
#include "BattleEngine.h"
#include "SoundManager.h"
#include "MatchResult.h"
#include "MatchQR.h"
#include "MatchResult.h"

//ENUMS


//GLOBALS
GameState currentState = STATE_MAIN_MENU;
PilotProfile pilot;
ActiveMech playerMech;
ActiveMech remoteMech;
DummyMechTarget dummyTarget;

std::vector<ChassisProfile> chassisProfiles;
std::vector<WeaponProfile> weaponProfiles;

MatchResult completedMatch;
bool completedMatchAvailable = false;

bool radarTargetLocked = false;

String newPilotName = "ACE";
int selectedNameChar = 0;

int selectedRadarIndex = 0;

String incomingChallengerName = "";
String incomingChallengerChassis = "";
bool hasIncomingChallenge = false;
uint8_t incomingChallengerChassisCode = 0;

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
bool matchWon = false;

bool matchResultRecorded = false;

char nameBuffer[9] = "        ";
int nameCursor;
int selectedFactionIndex;

int selectedBattleWeaponIndex = 0;
bool battleScreenDrawn = false;

bool localReadyForNextRound = false;
bool remoteReadyForNextRound = false;

int battleResultsPage = 0;

bool challengeAdvertisementStarted = false;

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
void handleIncomingChallengeScreen();
void handleMultiplayerBattleScreen();
void handleBattleResultsScreen();
void handleBattleVictoryScreen();
void handleBattleDefeatScreen();
void handleMatchQrScreen();

//------Helpers
void returnToMainMenu() {
  //inBattleTest = false;
  //inOptionsMenu = false;
  //inCreditsScreen = false;
  matchWon = false;
  currentState = STATE_MAIN_MENU;
  drawMainMenu(selectedMenuIndex);
}

bool buildRemoteMech(
    ActiveMech &mech,
    const String &pilotName,
    uint8_t chassisCode
);

bool buildRemoteMech(
    ActiveMech &mech,
    const String &pilotName,
    uint8_t chassisCode
) {
  PilotProfile remotePilot;
  remotePilot.pilotName = pilotName;

  BadgeConfig remoteBadge;

  switch (chassisCode) {
    case 1:
      remoteBadge.chassisId = "PEST";
      break;

    case 2:
      remoteBadge.chassisId = "CREEPER";
      break;

    case 3:
      remoteBadge.chassisId = "PATHFINDER";
      break;

    case 4:
      remoteBadge.chassisId = "DOZER";
      break;

    default:
      Serial.println("Remote mech build failed: unknown chassis code");
      return false;
  }

  const ChassisProfile *remoteChassis =
      findChassisById(chassisProfiles, remoteBadge.chassisId);

  if (remoteChassis == nullptr) {
    Serial.print("Remote chassis not found: ");
    Serial.println(remoteBadge.chassisId);
    return false;
  }

  return buildActiveMech(
      mech,
      remotePilot,
      remoteBadge,
      remoteChassis,
      weaponProfiles
  );
}

const ActiveLocation *findMechLocationForUI(
    const ActiveMech &mech,
    const String &locationId
) {
  for (const ActiveLocation &location : mech.locations) {
    if (location.id == locationId) {
      return &location;
    }
  }

  return nullptr;
}

void testBuzzer() {
    tone(BUZZER_PIN, 1200, 150);
    delay(200);
    tone(BUZZER_PIN, 800, 200);
}

void drawMechStatusTable(
    const ActiveMech &mech,
    const String &title
) {
  static const char *locationIds[] = {
    "HEAD", "CT", "LT", "RT",
    "LA", "RA", "LL", "RL"
  };

  tft.fillScreen(ST77XX_BLACK);

  tft.setTextColor(ST77XX_YELLOW);
  tft.setTextSize(2);
  tft.setCursor(15, 10);
  tft.println(title);

  tft.setTextSize(1);
  tft.setTextColor(ST77XX_CYAN);

  tft.setCursor(20, 42);
  tft.println("LOC    ARMOR       STRUCTURE");

  for (int i = 0; i < 8; i++) {
    const ActiveLocation *location =
        findMechLocationForUI(mech, locationIds[i]);

    int y = 62 + (i * 17);

    tft.setCursor(20, y);
    tft.setTextColor(ST77XX_WHITE);
    tft.print(locationIds[i]);

    if (location == nullptr) {
      tft.setCursor(85, y);
      tft.println("MISSING");
      continue;
    }

    uint16_t armorColor = ST77XX_GREEN;

    if (location->currentArmor <= 0) {
      armorColor = ST77XX_RED;
    } else if (
        location->currentArmor * 2 <=
        location->maxArmor
    ) {
      armorColor = ST77XX_YELLOW;
    }

    tft.setTextColor(armorColor);
    tft.setCursor(85, y);
    tft.print(location->currentArmor);
    tft.print("/");
    tft.print(location->maxArmor);

    uint16_t structureColor = ST77XX_GREEN;

    if (location->currentStructure <= 0) {
      structureColor = ST77XX_RED;
    } else if (
        location->currentStructure * 2 <=
        location->maxStructure
    ) {
      structureColor = ST77XX_YELLOW;
    }

    tft.setTextColor(structureColor);
    tft.setCursor(185, y);
    tft.print(location->currentStructure);
    tft.print("/");
    tft.println(location->maxStructure);
  }

  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(20, 218);
  tft.println("< RESULTS     ENEMY >");
}

//============SETUP()
void setup() {
  Serial.begin(115200);
  delay(1500);

  soundSetup(BUZZER_PIN);
  playSound(SoundEffect::LASER);

  randomSeed(esp_random());

  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS mount failed");
    return;
  }

  BadgeConfig badge;

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
  showBootSequence();

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

    case STATE_INCOMING_CHALLENGE:
      handleIncomingChallengeScreen();
      break;

    case STATE_MULTIPLAYER_BATTLE:
      handleMultiplayerBattleScreen();
      break;

    case STATE_BATTLE_RESULTS:
      handleBattleResultsScreen();
      break;

    case STATE_BATTLE_VICTORY:
      handleBattleVictoryScreen();
      break;

    case STATE_BATTLE_DEFEAT:
      handleBattleDefeatScreen();
      break;

    case STATE_MATCH_QR:
      handleMatchQrScreen();
      break;
  }

soundUpdate();

if (bleHasIncomingChallenge()) {
  CpecChallengePacket challenge = bleGetIncomingChallenge();

  // Only allow a new challenge while browsing RADAR.
  if (currentState == STATE_RADAR) {
    incomingChallengerName = challenge.challengerName;
    incomingChallengerChassis =
        chassisNameFromCode(challenge.chassisId);
    incomingChallengerChassisCode = challenge.chassisId;

    hasIncomingChallenge = true;
    currentState = STATE_INCOMING_CHALLENGE;

    Serial.println("Challenge accepted by state gate: entering incoming screen");
  } else {
    Serial.println("Challenge packet ignored: badge is not in RADAR");
  }

  // Consume it regardless of current state.
  bleClearIncomingChallenge();
}

if (bleHasIncomingAccept() && currentState == STATE_CHALLENGE_SENT) {
  CpecAcceptPacket accept = bleGetIncomingAccept();

  Serial.print("Challenge accepted by: ");
  Serial.println(accept.accepterName);

  bleClearIncomingAccept();

  if (!buildRemoteMech(
        remoteMech,
        accept.accepterName,
        accept.chassisId)) {

  Serial.println("Could not build accepting mech");
  return;
}

matchResultRecorded = false;
startMultiplayerBattle(playerMech, remoteMech);

  currentState = STATE_MULTIPLAYER_BATTLE;
}

if (bleHasIncomingTurn() &&
    currentState == STATE_MULTIPLAYER_BATTLE) {

  CpecTurnPacket turnPacket = bleGetIncomingTurn();

  if (turnPacket.round == getBattleRound()) {
    submitRemoteTurn(turnPacket.weaponSlot);

    Serial.print("Remote weapon received: ");
    Serial.println(turnPacket.weaponSlot);
  } else {
    Serial.print("Ignored turn for round ");
    Serial.println(turnPacket.round);
  }

  bleClearIncomingTurn();
}

if (bleHasIncomingReady()) {
  CpecReadyPacket ready = bleGetIncomingReady();

  if (currentState == STATE_BATTLE_RESULTS &&
      ready.nextRound == getBattleRound()) {

    remoteReadyForNextRound = true;

    Serial.print("Remote ready for round ");
    Serial.println(ready.nextRound);
  }

  bleClearIncomingReady();
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
    challengeAdvertisementStarted = false;
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

void handleIncomingChallengeScreen() {
  static bool screenDrawn = false;
  
  tft.fillScreen(ST77XX_BLACK);

  tft.setTextColor(ST77XX_YELLOW);
  tft.setTextSize(2);
  tft.setCursor(20, 20);
  tft.println("INCOMING");
  tft.setCursor(20, 50);
  tft.println("CHALLENGE");

  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(1);
  tft.setCursor(20, 95);
  tft.print("FROM: ");
  tft.println(incomingChallengerName);

  tft.setCursor(20, 115);
  tft.print("MECH: ");
  tft.println(incomingChallengerChassis);

  tft.setCursor(20, 190);
  tft.println("A = ACCEPT");
  tft.setCursor(20, 210);
  tft.println("B = DECLINE");

if (digitalRead(BTN_A) == LOW) {

  CpecAdvertisedPilot acceptPilot;
  acceptPilot.pilotName = pilot.pilotName;
  acceptPilot.chassisId = chassisCodeFromId(playerMech.badge.chassisId);

  bleAdvertiseAccept(acceptPilot);

  bleClearIncomingChallenge();
  hasIncomingChallenge = false;

  if (!buildRemoteMech(
        remoteMech,
        incomingChallengerName,
        incomingChallengerChassisCode)) {

  Serial.println("Could not build challenger mech");
  return;
}

matchResultRecorded = false;
startMultiplayerBattle(playerMech, remoteMech);

  currentState = STATE_MULTIPLAYER_BATTLE;

  delay(180);
  return;
}

  if (digitalRead(BTN_B) == LOW) {
    hasIncomingChallenge = false;
    currentState = STATE_RADAR;
    drawRadarScreen(selectedRadarIndex);
    delay(180);
    return;
  }
}

void handleMultiplayerBattleScreen() {
  int weaponCount = playerMech.weapons.size();

  static bool announcedBattleState = false;

  if (!announcedBattleState) {
  Serial.println("*** ENTERED MULTIPLAYER BATTLE ***");
  announcedBattleState = true;
}

if (!battleScreenDrawn) {

  if (weaponCount == 0) {
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextColor(ST77XX_RED);
    tft.setTextSize(2);
    tft.setCursor(20, 20);
    tft.println("NO WEAPONS");
    return;
  }

  if (selectedBattleWeaponIndex < 0) {
    selectedBattleWeaponIndex = weaponCount - 1;
  }

  if (selectedBattleWeaponIndex >= weaponCount) {
    selectedBattleWeaponIndex = 0;
  }

  tft.fillScreen(ST77XX_BLACK);

  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(2);
  tft.setCursor(20, 15);
  tft.print("ROUND ");
  tft.println(getBattleRound());

  tft.setTextSize(1);

  for (int i = 0; i < weaponCount && i < 6; i++) {
    int y = 55 + (i * 22);

    tft.setCursor(20, y);

    if (i == selectedBattleWeaponIndex) {
      tft.setTextColor(ST77XX_YELLOW);
      tft.print("> ");
    } else {
      tft.setTextColor(ST77XX_GREEN);
      tft.print("  ");
    }

    const ActiveWeapon &activeWeapon =
    playerMech.weapons[i];

    tft.print(activeWeapon.weapon.displayName);

    if (activeWeapon.maxAmmo >= 0) {
      tft.print(" [");
      tft.print(activeWeapon.currentAmmo);
      tft.print("]");
    }

    tft.println();
  }
  battleScreenDrawn = true;
}
  const BattleTurn &turn = getBattleTurn();

if (battleTurnReady()) {
  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(20, 185);
  tft.println("RESOLVING TURN...");

  delay(500);

  resolveBattleTurn();

  if (isLocalMechDestroyed() && isRemoteMechDestroyed()) {
  // Dirty alpha: treat a simultaneous kill as defeat for both.
  currentState = STATE_BATTLE_DEFEAT;
  battleScreenDrawn = false;
  return;
}

if (isRemoteMechDestroyed()) {
  currentState = STATE_BATTLE_VICTORY;
  battleScreenDrawn = false;
  return;
}

if (isLocalMechDestroyed()) {
  currentState = STATE_BATTLE_DEFEAT;
  battleScreenDrawn = false;
  return;
}

  Serial.println("Turn resolved");

  battleResultsPage = 0;
  battleScreenDrawn = false;
  currentState = STATE_BATTLE_RESULTS;
  return;
}

  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(20, 205);

  if (turn.localSubmitted) {
    tft.println("CHOICE LOCKED");
  } else {
    tft.println("UP/DOWN SELECT  A=LOCK");
  }

  if (!turn.localSubmitted && digitalRead(BTN_UP) == LOW) {
    selectedBattleWeaponIndex--;
    battleScreenDrawn = false;
    delay(180);
    return;
  }

  if (!turn.localSubmitted && digitalRead(BTN_DOWN) == LOW) {
    selectedBattleWeaponIndex++;
    battleScreenDrawn = false;
    delay(180);
    return;
  }

  if (!turn.localSubmitted && digitalRead(BTN_A) == LOW) {
    ActiveWeapon &selectedWeapon =
    playerMech.weapons[selectedBattleWeaponIndex];

  if (selectedWeapon.maxAmmo >= 0 &&
      selectedWeapon.currentAmmo <= 0) {

    Serial.println("Cannot fire: weapon is out of ammo");

    tft.setTextColor(ST77XX_RED);
    tft.setCursor(20, 185);
    tft.println("OUT OF AMMO");

    while (digitalRead(BTN_A) == LOW) {
      delay(10);
    }

    delay(250);
    battleScreenDrawn = false;
    return;
  }

    submitLocalTurn(selectedBattleWeaponIndex);

    bleAdvertiseTurn(
        getBattleRound(),
        selectedBattleWeaponIndex
    );

    Serial.print("Local weapon selected: ");
    Serial.println(selectedBattleWeaponIndex);
    battleScreenDrawn = false;
    delay(180);
    return;
  }

  if (digitalRead(BTN_B) == LOW) {
    currentState = STATE_RADAR;
    drawRadarScreen(selectedRadarIndex);
    announcedBattleState = false;
    delay(180);
    return;
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
  static bool screenDrawn = false;
if (!challengeAdvertisementStarted) {
  CpecAdvertisedPilot challengePilot;
  challengePilot.pilotName = pilot.pilotName;
  challengePilot.chassisId =
      chassisCodeFromId(playerMech.badge.chassisId);

  Serial.println("Starting challenge advertisement");
  bleAdvertiseChallenge(challengePilot);

  challengeAdvertisementStarted = true;
}

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

if (digitalRead(BTN_A) == LOW) {
  Serial.println("A PRESSED ON CHALLENGE SENT");
  CpecAdvertisedPilot challengePilot;
  challengePilot.pilotName = pilot.pilotName;
  challengePilot.chassisId = chassisCodeFromId(playerMech.badge.chassisId);

  bleAdvertiseChallenge(challengePilot);

  delay(180);
  return;
}

  if (digitalRead(BTN_B) == LOW) {
      challengeAdvertisementStarted = false;
      currentState = STATE_RADAR;
      drawRadarScreen(selectedRadarIndex);
      delay(180);
      return;
  }
}

void handleBattleResultsScreen() {
  static bool screenDrawn = false;

  if (!screenDrawn) {
    const BattleRoundResult &result =
        getLastBattleResult();

    if (battleResultsPage == 0) {
      tft.fillScreen(ST77XX_BLACK);

      tft.setTextColor(ST77XX_YELLOW);
      tft.setTextSize(2);
      tft.setCursor(15, 10);
      tft.print("ROUND ");
      tft.print(result.round);
      tft.println(" RESULTS");

      tft.setTextColor(ST77XX_CYAN);
      tft.setTextSize(1);
      tft.setCursor(220, 18);
      tft.print("HEAT ");
      tft.println(playerMech.currentHeat);

      tft.setTextSize(1);

      // Your attack
      tft.setTextColor(ST77XX_GREEN);
      tft.setCursor(15, 52);
      tft.print("YOU: ");
      tft.println(result.localWeaponName);

      tft.setCursor(15, 70);
      tft.print("ROLL: ");
      tft.print(result.localAttackRoll);
      tft.print(" / ");
      tft.print(result.localTargetNumber);

      if (result.localHit) {
        tft.println("  HIT");
      } else {
        tft.println("  MISS");
      }

      tft.setCursor(15, 88);
      tft.print("LOC: ");
      tft.println(result.localHitLocation);

      tft.setCursor(15, 106);
      tft.print("DMG: ");
      tft.println(result.localDamageDealt);

      tft.setCursor(15, 124);
      tft.print("HEAT +");
      tft.println(result.localHeatAdded);

      if (result.localAmmoRemaining >= 0) {
        tft.setCursor(15, 142);
        tft.print("AMMO: ");
        tft.println(result.localAmmoRemaining);
      }

      // Enemy attack
      tft.setTextColor(ST77XX_CYAN);
      tft.setCursor(165, 52);
      tft.print("ENEMY: ");
      tft.println(result.remoteWeaponName);

      tft.setCursor(165, 70);
      tft.print("ROLL: ");
      tft.print(result.remoteAttackRoll);
      tft.print(" / ");
      tft.print(result.remoteTargetNumber);

      if (result.remoteHit) {
        tft.println("  HIT");
      } else {
        tft.println("  MISS");
      }

      tft.setCursor(165, 88);
      tft.print("LOC: ");
      tft.println(result.remoteHitLocation);

      tft.setCursor(165, 106);
      tft.print("DMG: ");
      tft.println(result.remoteDamageDealt);

      tft.setCursor(165, 124);
      tft.print("HEAT +");
      tft.println(result.remoteHeatAdded);

      if (result.remoteAmmoRemaining >= 0) {
        tft.setCursor(165, 142);
        tft.print("AMMO: ");
        tft.println(result.remoteAmmoRemaining);
      }

      tft.setTextColor(ST77XX_WHITE);
      tft.setCursor(15, 185);

      if (localReadyForNextRound) {
        tft.println("READY - WAITING FOR ENEMY");
      } else {
        tft.println("A = READY");
      }

      tft.setTextColor(ST77XX_CYAN);
      tft.setCursor(15, 218);
      tft.println("RIGHT = YOUR STATUS");
    }

    if (battleResultsPage == 1) {
      drawMechStatusTable(
          playerMech,
          "YOUR MECH"
      );
    }

    if (battleResultsPage == 2) {
      drawMechStatusTable(
          remoteMech,
          "ENEMY MECH"
      );
    }

    screenDrawn = true;
  }

  // LEFT changes page.
  if (digitalRead(BTN_LEFT) == LOW) {
    battleResultsPage--;

    if (battleResultsPage < 0) {
      battleResultsPage = 2;
    }

    screenDrawn = false;

    while (digitalRead(BTN_LEFT) == LOW) {
      delay(10);
    }

    return;
  }

  // RIGHT changes page.
  if (digitalRead(BTN_RIGHT) == LOW) {
    battleResultsPage++;

    if (battleResultsPage > 2) {
      battleResultsPage = 0;
    }

    screenDrawn = false;

    while (digitalRead(BTN_RIGHT) == LOW) {
      delay(10);
    }

    return;
  }

  // Mark local player ready.
  if (!localReadyForNextRound &&
      digitalRead(BTN_A) == LOW) {

    localReadyForNextRound = true;

    bleAdvertiseReady(getBattleRound());

    Serial.print("Local ready for round ");
    Serial.println(getBattleRound());

    while (digitalRead(BTN_A) == LOW) {
      delay(10);
    }

    screenDrawn = false;
    return;
  }

  // Advance only when both badges are ready.
  if (localReadyForNextRound &&
      remoteReadyForNextRound) {

    localReadyForNextRound = false;
    remoteReadyForNextRound = false;

    battleResultsPage = 0;
    screenDrawn = false;
    battleScreenDrawn = false;

    currentState = STATE_MULTIPLAYER_BATTLE;

    delay(50);
    return;
  }
}

void handleBattleVictoryScreen() {
  if (!matchResultRecorded) {
  pilot.battles++;
  pilot.wins++;
  pilot.kills++;

  savePilotProfile(pilot);

completedMatch.version = 1;

completedMatch.localPilotId = pilot.pilotId;
completedMatch.localPilotName = pilot.pilotName;

completedMatch.remotePilotId = remoteMech.pilot.pilotId;
completedMatch.remotePilotName = remoteMech.pilot.pilotName;

completedMatch.result = "W";
completedMatch.rounds = getBattleRound() - 1;

completedMatch.localChassisId =
    playerMech.badge.chassisId;

completedMatch.remoteChassisId =
    remoteMech.badge.chassisId;

completedMatchAvailable = true;

Serial.print("Match payload: ");
Serial.println(encodeMatchResult(completedMatch));

  Serial.println("Victory recorded:");
  Serial.print("Battles: ");
  Serial.println(pilot.battles);
  Serial.print("Wins: ");
  Serial.println(pilot.wins);
  Serial.print("Kills: ");
  Serial.println(pilot.kills);

  matchResultRecorded = true;
}

  static bool screenDrawn = false;

  if (!screenDrawn) {
    tft.fillScreen(ST77XX_BLACK);

    tft.setTextColor(ST77XX_GREEN);
    tft.setTextSize(3);
    tft.setCursor(60, 45);
    tft.println("VICTORY");

    tft.setTextSize(1);
    tft.setCursor(70, 110);
    tft.println("ENEMY MECH DESTROYED");

    tft.setCursor(80, 205);
    tft.println("A = SHOW QR");

    screenDrawn = true;
  }

  if (digitalRead(BTN_A) == LOW) {
    while (digitalRead(BTN_A) == LOW) {
      delay(10);
    }

    screenDrawn = false;
    currentState = STATE_MATCH_QR;

    delay(50);
    return;
  }
}

void handleBattleDefeatScreen() {
if (!matchResultRecorded) {
  pilot.battles++;
  pilot.losses++;
  pilot.deaths++;

  savePilotProfile(pilot);

  completedMatch.version = 1;

  completedMatch.localPilotId = pilot.pilotId;
  completedMatch.localPilotName = pilot.pilotName;

  completedMatch.remotePilotId = remoteMech.pilot.pilotId;
  completedMatch.remotePilotName = remoteMech.pilot.pilotName;

  completedMatch.result = "L";
  completedMatch.rounds = getBattleRound() - 1;

  completedMatch.localChassisId =
      playerMech.badge.chassisId;

  completedMatch.remoteChassisId =
      remoteMech.badge.chassisId;

  completedMatchAvailable = true;

  Serial.print("Match payload: ");
  Serial.println(encodeMatchResult(completedMatch));

  matchResultRecorded = true;
}

  static bool screenDrawn = false;

  if (!screenDrawn) {
    tft.fillScreen(ST77XX_BLACK);

    tft.setTextColor(ST77XX_RED);
    tft.setTextSize(3);
    tft.setCursor(65, 45);
    tft.println("DEFEAT");

    tft.setTextSize(1);
    tft.setCursor(75, 110);
    tft.println("YOUR MECH WAS DESTROYED");

    tft.setCursor(80, 205);
    tft.println("A = SHOW QR");

    screenDrawn = true;
  }

  if (digitalRead(BTN_A) == LOW) {
    while (digitalRead(BTN_A) == LOW) {
      delay(10);
    }

    screenDrawn = false;
    currentState = STATE_MATCH_QR;

    delay(50);
    return;
  }
}

void playWeaponSound(const String& weaponName)
{
    if (weaponName.indexOf("Laser") >= 0 ||
        weaponName.indexOf("Pulse") >= 0) {
        playSound(SoundEffect::LASER);
    }
    else if (weaponName.indexOf("Missile") >= 0 ||
             weaponName.indexOf("ML5") >= 0 ||
             weaponName.indexOf("SRM") >= 0 ||
             weaponName.indexOf("LRM") >= 0) {
        playSound(SoundEffect::MISSILE);
    }
    else if (weaponName.indexOf("Autocannon") >= 0 ||
             weaponName.indexOf("AC/") >= 0) {
        playSound(SoundEffect::AUTOCANNON);
    }
    else if (weaponName.indexOf("MG") >= 0 ||
             weaponName.indexOf("Chaingun") >= 0) {
        playSound(SoundEffect::MACHINE_GUN);
    }
}

void handleMatchQrScreen() {
  static bool screenDrawn = false;

  if (!screenDrawn) {
    if (!completedMatchAvailable) {
      tft.fillScreen(ST77XX_BLACK);
      tft.setTextColor(ST77XX_RED);
      tft.setTextSize(2);
      tft.setCursor(20, 40);
      tft.println("NO MATCH DATA");

      tft.setTextSize(1);
      tft.setCursor(20, 210);
      tft.println("B = MAIN MENU");
    } else {
      String payload =
          encodeMatchResult(completedMatch);

      Serial.print("Drawing QR payload: ");
      Serial.println(payload);

      drawMatchQr(tft, payload);

      // Keep this clear of the QR itself.
      tft.setTextColor(ST77XX_BLACK);
      tft.setTextSize(1);
      tft.setCursor(10, 225);
      tft.println("B = MAIN MENU");
    }

    screenDrawn = true;
  }

  if (digitalRead(BTN_B) == LOW) {
    while (digitalRead(BTN_B) == LOW) {
      delay(10);
    }

    screenDrawn = false;
    completedMatchAvailable = false;

    currentState = STATE_MAIN_MENU;
    selectedMenuIndex = 0;
    drawMainMenu(selectedMenuIndex);

    delay(50);
    return;
  }
}