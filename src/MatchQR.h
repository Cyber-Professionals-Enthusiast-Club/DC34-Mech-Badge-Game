#pragma once

#include <Arduino.h>
#include <Adafruit_ST7789.h>

void drawMatchQr(
    Adafruit_ST7789 &tft,
    const String &payload
);