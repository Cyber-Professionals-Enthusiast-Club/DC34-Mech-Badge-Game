#include "PilotRecordUI.h"
#include "CPEC_BattleMech_TextPort.h"

void drawPilotRecordScreen(const PilotProfile &pilot) {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextWrap(false);

  tft.setTextColor(ST77XX_YELLOW);
  tft.setTextSize(2);
  tft.setCursor(20, 10);
  tft.println("PILOT RECORD");

  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(1);

  int y = 45;

  tft.setCursor(20, y);
  tft.print("Pilot: ");
  tft.println(pilot.pilotName);
  y += 14;

  tft.setCursor(20, y);
  tft.print("Faction: ");
  tft.println(pilot.faction);
  y += 18;

  tft.setCursor(20, y);
  tft.print("Level: ");
  tft.println(pilot.level);
  y += 14;

  tft.setCursor(20, y);
  tft.print("XP: ");
  tft.println(pilot.xp);
  y += 14;

  tft.setCursor(20, y);
  tft.print("Skill Points: ");
  tft.println(pilot.skillPoints);
  y += 18;

  tft.setCursor(20, y);
  tft.print("Wins: ");
  tft.println(pilot.wins);
  y += 14;

  tft.setCursor(20, y);
  tft.print("Losses: ");
  tft.println(pilot.losses);
  y += 14;

  tft.setCursor(20, y);
  tft.print("Kills: ");
  tft.println(pilot.kills);
  y += 14;

  tft.setCursor(20, y);
  tft.print("Deaths: ");
  tft.println(pilot.deaths);
  y += 14;

  tft.setCursor(20, y);
  tft.print("Battles: ");
  tft.println(pilot.battles);

  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(20, 220);
  tft.println("B = BACK");
}