#include "MatchQR.h"

#include <qrcode.h>

void drawMatchQr(
    Adafruit_ST7789 &tft,
    const String &payload
) {
  constexpr uint8_t qrVersion = 6;
  constexpr uint8_t scale = 3;

  QRCode qrcode;

  uint8_t qrcodeData[
      qrcode_getBufferSize(qrVersion)
  ];

  qrcode_initText(
      &qrcode,
      qrcodeData,
      qrVersion,
      ECC_LOW,
      payload.c_str()
  );

  int qrPixels = qrcode.size * scale;

  int startX = (320 - qrPixels) / 2;
  int startY = (240 - qrPixels) / 2;

  tft.fillScreen(ST77XX_WHITE);

  for (uint8_t y = 0; y < qrcode.size; y++) {
    for (uint8_t x = 0; x < qrcode.size; x++) {
      uint16_t color =
          qrcode_getModule(&qrcode, x, y)
              ? ST77XX_BLACK
              : ST77XX_WHITE;

      tft.fillRect(
          startX + (x * scale),
          startY + (y * scale),
          scale,
          scale,
          color
      );
    }
  }
}