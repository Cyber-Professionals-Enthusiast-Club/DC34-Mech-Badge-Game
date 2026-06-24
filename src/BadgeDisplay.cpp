#include "BadgeDisplay.h"
#include "CPEC_BattleMech_TextPort.h"

#include <Arduino.h>
#include <LittleFS.h>
#include <TJpg_Decoder.h>

void initDisplay() {
  tft.init(240, 320);
  tft.setRotation(1);
  tft.fillScreen(ST77XX_BLACK);
}

bool tftOutput(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
  tft.drawRGBBitmap(x, y, bitmap, w, h);
  return true;
}

void showBootScreen() {
  Serial.println("showBootScreen start");

  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS mount failed");
    return;
  }

  Serial.println("LittleFS mounted");

  if (!LittleFS.exists("/boot.jpg")) {
    Serial.println("boot.jpg not found");
    return;
  }

  Serial.println("boot.jpg found");

  TJpgDec.setCallback(tftOutput);
  TJpgDec.setJpgScale(1);

  bool ok = TJpgDec.drawFsJpg(0, 0, "/boot.jpg", LittleFS);

  Serial.print("drawFsJpg returned: ");
  Serial.println(ok ? "true" : "false");
}