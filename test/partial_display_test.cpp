#include "partial_display.h"
#include <assert.h>
#include <stdio.h>

int main() {
  TFT_eSPI lcd, reference;
  PartialDisplay display(lcd);
  assert(display.begin());
  uint16_t digitColor = 0xFFFF;
  auto scene = [&](TFT_eSPI &canvas) {
    canvas.fillRect(0, 0, 320, 240, 0x0862);
    canvas.fillRect(110, 70, 13, 25, digitColor); // Liczba przechodzi przez granice pasow.
    canvas.fillRect(319, 239, 1, 1, 0xF800); // Prawy dolny rog.
  };
  display.present(scene);
  scene(reference);
  assert(lcd.pixels == reference.pixels);
  assert(lcd.pixelsWritten == 320U * 240U);
  lcd.pixelsWritten = 0;
  display.present(scene);
  assert(lcd.pixelsWritten == 0); // Polling bez zmian nie dotyka LCD.
  digitColor = 0x07E0;
  display.present(scene);
  scene(reference);
  assert(lcd.pixels == reference.pixels);
  assert(lcd.pixelsWritten > 0 && lcd.pixelsWritten <= 4U * 16U * 16U);
  lcd.pixelsWritten = 0;
  // Zmiana widoku tez nie moze zostawic starych cyfr ani czyscic LCD oddzielna klatka.
  auto nextScene = [](TFT_eSPI &canvas) { canvas.fillRect(0, 0, 320, 240, 0x0862); };
  display.present(nextScene);
  nextScene(reference);
  assert(lcd.pixels == reference.pixels);
  assert(lcd.pixelsWritten > 0 && lcd.pixelsWritten < 320U * 240U);
  puts("OK: strip coordinates, local transfer, unchanged frame, view transition");
}
