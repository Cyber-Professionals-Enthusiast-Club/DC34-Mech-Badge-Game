#pragma once

#include <Adafruit_ST7789.h>
#include <SPI.h>

extern Adafruit_ST7789 tft;

#define BTN_UP      7
#define BTN_DOWN    10
#define BTN_LEFT    8
#define BTN_RIGHT   9
#define BTN_SELECT  1
#define BTN_START   2
#define BTN_A       11
#define BTN_B       14

void badgeSetup();
void badgeLoop();
void showBootSequence();