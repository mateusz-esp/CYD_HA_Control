#pragma once
#include <TFT_eSPI.h>

namespace Typography {
struct Glyph {
  uint16_t code;
  uint32_t offset;
  uint8_t width, height, advance;
  int8_t x, y;
};
struct Font {
  const uint8_t *pixels;
  const Glyph *glyphs;
  uint16_t count;
  uint8_t ascent, height;
};
extern const Font Small, Body, Label, Title, Number;
enum class Align { Left, Center, Right };
int width(const char *text, const Font &font);
void text(TFT_eSPI &canvas, int x, int y, const char *value, const Font &font,
          uint16_t foreground, uint16_t background, int maxWidth = 320,
          Align align = Align::Left);
}
