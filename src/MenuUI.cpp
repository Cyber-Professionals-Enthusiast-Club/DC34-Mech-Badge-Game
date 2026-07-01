#include "MenuUI.h"
#include "CPEC_BattleMech_TextPort.h"

static const char* MENU_ITEMS[] = {
  "STATUS",
  "BATTLE TEST",
  "RADAR",
  "PILOT RECORD",
  "OPTIONS"
};

static const int MENU_COUNT = 5;

void drawMainMenu(int selectedIndex) {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextWrap(false);

  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(2);
  tft.setCursor(20, 15);
  tft.println("MAIN MENU");

  tft.setTextSize(2);

  for (int i = 0; i < MENU_COUNT; i++) {
    int y = 55 + (i * 32);

    tft.setCursor(30, y);

    if (i == selectedIndex) {
      tft.setTextColor(ST77XX_YELLOW);
      tft.print("> ");
    } else {
      tft.setTextColor(ST77XX_GREEN);
      tft.print("  ");
    }

    tft.println(MENU_ITEMS[i]);
  }

  tft.setTextColor(ST77XX_CYAN);
  tft.setTextSize(1);
  tft.setCursor(20, 220);
  tft.println("UP/DOWN SELECT  A=OK  B=BACK");
}