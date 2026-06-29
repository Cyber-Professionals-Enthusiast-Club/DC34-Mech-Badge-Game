#include "PlayerBuilderUI.h"
#include "CPEC_BattleMech_TextPort.h"
#include "ProfileLoader.h"
#include "MenuUI.h"
#include "Gamestate.h"

//EXTERNAL CALLS
extern PilotProfile pilot;
extern String newPilotName;
extern int selectedFactionIndex;
extern GameState currentState;
extern int selectedMenuIndex;

extern const char* FACTIONS[];
extern const int FACTION_COUNT;

extern char nameBuffer[9];
extern int nameCursor;

void drawPlayerNameScreen() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_YELLOW);
  tft.setTextSize(2);
  tft.setCursor(20, 20);
  tft.println("NEW PILOT");

  tft.setTextColor(ST77XX_GREEN);
  tft.setCursor(20, 80);
    int nameX = 130;
    int nameY = 80;

    tft.setCursor(20, nameY);
    tft.print("CALLSIGN:");

    tft.setCursor(nameX, nameY);
    tft.println(nameBuffer);

    tft.setCursor(nameX + (nameCursor * 12), nameY + 20);
    tft.println("^");

  tft.setTextSize(1);
  tft.setCursor(20, 205);
  tft.println("UP/DOWN = Character. Left/Right = Cursor Position");
  tft.setCursor(20, 220);
  tft.println("A=NEXT");
}

