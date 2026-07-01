#include "RadarUI.h"
#include "BLEComms.h"
#include "CPEC_BattleMech_TextPort.h"


void drawRadarScreen(int selectedIndex) {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextWrap(false);

  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(2);
  tft.setCursor(20, 15);
  tft.println("RADAR");

  tft.setTextSize(1);

  int count = getNearbyBadgeCount();

  if (count == 0) {
    tft.setCursor(20, 60);
    tft.println("No contacts detected");
  } else {
    for (int i = 0; i < count && i < 6; i++) {
      NearbyBadge badge = getNearbyBadge(i);

      int y = 55 + (i * 22);

      tft.setCursor(20, y);

      if (i == selectedIndex) {
        tft.setTextColor(ST77XX_YELLOW);
        tft.print("> ");
      } else {
        tft.setTextColor(ST77XX_GREEN);
        tft.print("  ");
      }

      tft.print(badge.name);

      tft.setCursor(220, y);
      tft.print("RSSI ");
      tft.println(badge.rssi);
  }

  tft.setTextColor(ST77XX_CYAN);
  tft.setTextSize(1);
  tft.setCursor(20, 220);
  tft.println("UP/DOWN SELECT  A=LOCK  B=BACK");
}
}
