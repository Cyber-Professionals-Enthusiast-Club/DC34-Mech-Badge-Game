#include "CockpitUI.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "CPEC_BattleMech_TextPort.h"

void drawCockpitStatusTFT(const ActiveMech &mech) {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextWrap(false);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_GREEN);

  int y = 5;

  tft.setCursor(5, y);
  tft.println("MECH STATUS");
  y += 14;


  tft.setCursor(5, y);
  tft.print("MECH: ");
  tft.println(mech.chassis.displayName);
  y += 10;

  tft.setCursor(5, y);
  tft.print("CLASS: ");
  tft.println(mech.chassis.mechClass);
  y += 10;

  tft.setCursor(5, y);
  tft.print("HEAT: ");
  tft.print(mech.currentHeat);
  tft.print("/");
  tft.println(mech.chassis.heatCapacity);
  y += 14;
  tft.print("");
  tft.println(" ARMOR/STRUCTURE");
  y += 11;

  for (const ActiveLocation &loc : mech.locations) {
    tft.setCursor(5, y);
    tft.print(loc.id);
    tft.print(" A:");
    tft.print(loc.currentArmor);
    tft.print("/");
    tft.print(loc.maxArmor);
    tft.print(" S:");
    tft.print(loc.currentStructure);
    tft.print("/");
    tft.print(loc.maxStructure);
    y += 10;
  }

  y += 6;
  tft.setCursor(5, y);
  tft.println("WEAPONS");
  y += 10;

  for (const ActiveWeapon &weapon : mech.weapons) {
    tft.setCursor(5, y);
    tft.print(weapon.slot);
    tft.print(" ");
    tft.print(weapon.weapon.displayName);
    tft.print(" D:");
    tft.print(weapon.weapon.damage);
    tft.print(" H:");
    tft.print(weapon.weapon.heat);
    y += 10;
  }
}