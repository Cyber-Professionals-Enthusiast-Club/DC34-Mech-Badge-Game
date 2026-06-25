#include "OptionsUI.h"
#include "CPEC_BattleMech_TextPort.h"

void drawOptionsMenu(int selectedIndex) {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(2);
  tft.setCursor(20, 15);
  tft.println("OPTIONS");

  tft.setCursor(30, 65);
  tft.setTextColor(selectedIndex == 0 ? ST77XX_YELLOW : ST77XX_GREEN);
  tft.print(selectedIndex == 0 ? "> " : "  ");
  tft.println("CREDITS");

  tft.setTextColor(ST77XX_CYAN);
  tft.setTextSize(1);
  tft.setCursor(20, 220);
  tft.println("A=SELECT  B=BACK");
}

void drawCreditsScreen() {
  tft.fillScreen(ST77XX_BLACK);

  tft.setTextColor(ST77XX_YELLOW);
  tft.setTextSize(2);
  tft.setCursor(20, 55);
  tft.println("CREDITS");

  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(1);
  tft.setCursor(20, 55);
  tft.println("CPEC Mech Fighters");
  tft.println("DEFCON 34 Badge");
  tft.println("");
  tft.println("Game Design: CPEC");
  tft.println("Firmware: CPEC Dev Team");
  tft.println("Mech Systems: Jordan");
  tft.println("");
  tft.println("B=BACK");
}