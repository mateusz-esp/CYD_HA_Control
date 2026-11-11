#pragma once
#include <TFT_eSPI.h>

// TPM408-2.8 / CYD: natywne 240x320, logicznie 320x240.
namespace PanelUI {
constexpr uint8_t ROTATION = 1; // 3 = poziomo obrocone o 180 stopni.
static_assert(ROTATION == 1 || ROTATION == 3, "Panel wymaga orientacji poziomej");
constexpr int WIDTH = 320, HEIGHT = 240;
constexpr int CARD_Y = 62, CARD_W = 96, CARD_H = 148;
constexpr uint16_t BG = 0xF79E, SURFACE = 0xFFFF, SHADOW = 0xE73C;
constexpr uint16_t BORDER = 0xE73D, BUTTON = 0xEF5D;
constexpr uint16_t TEXT = 0x1947, MUTED = 0x738F;
constexpr uint16_t AMBER = 0xC3A6, BLUE = 0x4359, TEAL = 0x23EE;
constexpr uint16_t ACCENTS[] = {AMBER, BLUE, TEAL};
constexpr uint16_t ACTIVE_BG[] = {0xF72B, 0xE73F, 0xDF9A};
constexpr uint16_t ICON_BG[] = {0xF72B, 0xE73F, 0xDF9A};
struct Rect {
  int x, y, w, h;
  bool contains(int px, int py) const { return px >= x && px < x + w && py >= y && py < y + h; }
};
constexpr int cardX(uint8_t index) { return 8 + index * 104; }
inline Rect homePower(uint8_t index) { return {cardX(index) + 8, 178, 80, 26}; }
constexpr Rect BACK = {8, 8, 36, 32}, POWER = {238, 8, 74, 32};
constexpr Rect MINUS = {154, 76, 36, 36}, PLUS = {196, 76, 36, 36};
constexpr Rect SAVE = {242, 76, 58, 36};
constexpr Rect PREVIOUS = {8, 201, 84, 24}, NEXT = {228, 201, 84, 24};
inline Rect optionRow(uint8_t row) { return {8, 48 + row * 38, 304, 34}; }
inline Rect fieldRect(uint8_t slot, uint8_t count) {
  return count == 1 ? Rect{8, 134, 304, 38} : Rect{8 + (slot % 2) * 156, 132 + (slot / 2) * 30, 148, 26};
}
inline bool hit(uint8_t index, int x, int y) {
  return x >= cardX(index) && x < cardX(index) + CARD_W &&
         y >= CARD_Y && y < CARD_Y + CARD_H;
}

inline void power(TFT_eSPI &c, int x, int y, uint16_t color, uint16_t background) {
  c.drawCircle(x, y, 5, color);
  // Przerwa na gorze okregu i pionowy trzpien symbolu zasilania.
  c.fillRect(x - 2, y - 6, 5, 3, background);
  c.drawLine(x, y - 7, x, y - 1, color);
}

// Ikony rysowane prymitywami: bez bitmap i dodatkowych alokacji.
inline void icon(TFT_eSPI &c, uint8_t device, int x, int y, uint16_t color) {
  if (device == 0) {
    for (int dx = -10; dx <= 6; dx += 8) {
      c.drawRoundRect(x + dx, y - 9, 5, 19, 2, color);
      c.drawFastVLine(x + dx + 2, y - 7, 15, color);
    }
    c.drawFastHLine(x - 12, y - 5, 3, color);
    c.drawFastHLine(x + 11, y + 5, 3, color);
    c.drawFastHLine(x - 8, y + 12, 17, color);
  } else if (device == 1) {
    c.drawLine(x, y - 13, x, y + 13, color);
    c.drawLine(x - 11, y - 7, x + 11, y + 7, color);
    c.drawLine(x - 11, y + 7, x + 11, y - 7, color);
    for (int sign : {-1, 1}) {
      c.drawLine(x, y + sign * 9, x - 4, y + sign * 5, color);
      c.drawLine(x, y + sign * 9, x + 4, y + sign * 5, color);
      c.drawLine(x + sign * 8, y + sign * 5, x + sign * 8, y, color);
      c.drawLine(x + sign * 8, y + sign * 5, x + sign * 3, y + sign * 6, color);
      c.drawLine(x + sign * 8, y - sign * 5, x + sign * 8, y, color);
      c.drawLine(x + sign * 8, y - sign * 5, x + sign * 3, y - sign * 6, color);
    }
  } else {
    c.drawRoundRect(x - 9, y - 12, 18, 24, 5, color);
    c.drawFastHLine(x - 6, y - 7, 12, color);
    c.fillTriangle(x, y - 3, x - 4, y + 3, x + 4, y + 3, color);
    c.fillCircle(x, y + 3, 4, color);
    c.drawFastVLine(x - 5, y + 12, 3, color);
    c.drawFastVLine(x + 5, y + 12, 3, color);
  }
}
}
