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
  tft.setCursor(20, 25);
  tft.println("CREDITS");

  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(1);
  tft.setCursor(30, 55);     
  tft.println("CPEC Mech Fighters- A DEFCON 34 Badge");
  tft.println("2026 Cybersecurity Enthusiasts and Professionals Club");
  tft.println("");
  tft.println("Game Design: dirtybert");
  tft.println("Graphics/UI): Artist");
  tft.println("Firmware: Ministry");
  tft.println("Artwork: Scriptk80");
  tft.println("Game Programming: sshinobi");
  tft.println("Web/Database: Meek the CyberFreak");
  tft.println("Hardware Design: Ministry, Brothercroo, Artist");
  tft.println("Communication Design: Aniziki");
  tft.println("Social Media: Scriptk80");
  tft.println("");
  tft.println("http://cpec.club");
  tft.println("");
  tft.println("B=BACK");
}