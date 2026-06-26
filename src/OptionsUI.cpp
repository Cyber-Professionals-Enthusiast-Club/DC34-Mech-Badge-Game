#include "OptionsUI.h"
#include "CPEC_BattleMech_TextPort.h"

void drawOptionsMenu(int selectedIndex) {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(2);
  tft.setCursor(20, 15);
  tft.println("OPTIONS");

  tft.setCursor(30, 45);
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
<<<<<<< HEAD
  tft.setCursor(20, 25);
=======
  tft.setCursor(20, 35);
>>>>>>> fef19dc40ec0186dbeb34a5ac7716417a8ac0c3f
  tft.println("CREDITS");

  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(1);
<<<<<<< HEAD
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
=======
  tft.setCursor(35, 55);
  tft.println("CPEC Mech Fighters - 2026");
  tft.setCursor(40, 75);
  tft.println("Game Design - dirtybert & Carl Freeman");
  tft.setCursor(40, 85);
  tft.println("UX/UI: Donakamote");
  tft.setCursor(40, 95);
  tft.println("Firmware: Ministry");
  tft.setCursor(40, 105);
  tft.println("Game Programming: sshinobi");
  tft.setCursor(40, 115);
  tft.println("Communications Testing: Aniziki");
  tft.setCursor(40, 125);
  tft.println("Web/Database: Meek the CyberFreak");
  tft.setCursor(40, 135);
  tft.println("Hardware Design: Brothercroo, Ministry");
  tft.setCursor(40, 145);
  tft.println("Badge Art/Silk Screening: Scriptk80");
  tft.setCursor(40, 155);
  tft.println("Social Media: Scriptk80");
  tft.setCursor(40, 165);
>>>>>>> fef19dc40ec0186dbeb34a5ac7716417a8ac0c3f
  tft.println("");
  tft.setCursor(40, 175);
  tft.println("http://cpec.club // IG: CPEC // X: CPEC.CLUB");
  tft.setCursor(20, 205);
  tft.println("B=BACK");
}