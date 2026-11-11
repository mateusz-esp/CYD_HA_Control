#include "ui_font.h"
#ifndef PROGMEM
#define PROGMEM
#endif

namespace Typography {
#include "../assets/fonts/fira_generated.h"
namespace {
uint16_t decode(const char *&text) {
  const uint8_t c = static_cast<uint8_t>(*text++);
  if (c < 0x80) return c;
  if ((c & 0xE0) == 0xC0 && *text) {
    const uint16_t value = ((c & 0x1F) << 6) | (static_cast<uint8_t>(*text++) & 0x3F);
    return value;
  }
  if ((c & 0xF0) == 0xE0 && text[0] && text[1]) {
    uint16_t value = (c & 0x0F) << 12;
    value |= (static_cast<uint8_t>(*text++) & 0x3F) << 6;
    value |= static_cast<uint8_t>(*text++) & 0x3F;
    return value;
  }
  return '?';
}
const Glyph *glyph(const Font &font, uint16_t code) {
  for (uint16_t i = 0; i < font.count; ++i) if (font.glyphs[i].code == code) return &font.glyphs[i];
  for (uint16_t i = 0; i < font.count; ++i) if (font.glyphs[i].code == '?') return &font.glyphs[i];
  return nullptr;
}
uint16_t blend(uint16_t front, uint16_t back, int coverage) {
  const int r = (((front >> 11) & 31) * coverage + ((back >> 11) & 31) * (15 - coverage) + 7) / 15;
  const int g = (((front >> 5) & 63) * coverage + ((back >> 5) & 63) * (15 - coverage) + 7) / 15;
  const int b = ((front & 31) * coverage + (back & 31) * (15 - coverage) + 7) / 15;
  return (r << 11) | (g << 5) | b;
}
void paint(TFT_eSPI &c, int x, int baseline, const Glyph &g, const Font &font, const uint16_t *palette) {
  const int top = baseline + g.y;
  const int origin = c.getOriginY();
  if (top + origin >= c.height() || top + g.height + origin <= 0) return;
  const int first = top + origin < 0 ? -top - origin : 0;
  const int last = top + g.height + origin > c.height() ? c.height() - top - origin : g.height;
  for (int row = first; row < last; ++row) {
    for (int col = 0; col < g.width; ++col) {
      const int pixel = row * g.width + col;
      const uint8_t packed = font.pixels[g.offset + pixel / 2];
      const uint8_t alpha = pixel & 1 ? packed & 15 : packed >> 4;
      if (alpha) c.drawPixel(x + g.x + col, top + row, palette[alpha]);
    }
  }
}
}
int width(const char *value, const Font &font) {
  int result = 0;
  while (*value) { const auto *g = glyph(font, decode(value)); if (g) result += g->advance; }
  return result;
}
void text(TFT_eSPI &c, int x, int y, const char *value, const Font &font,
          uint16_t front, uint16_t back, int maxWidth, Align align) {
  const int fullWidth = width(value, font);
  const Glyph *ellipsis = glyph(font, 0x2026);
  int visibleWidth = fullWidth;
  const char *end = nullptr;
  if (fullWidth > maxWidth) {
    visibleWidth = 0;
    const char *cursor = value;
    end = value;
    while (*cursor) {
      const auto *g = glyph(font, decode(cursor));
      if (!g) continue;
      if (visibleWidth + g->advance + (ellipsis ? ellipsis->advance : 0) > maxWidth) break;
      visibleWidth += g->advance;
      end = cursor;
    }
    if (ellipsis) visibleWidth += ellipsis->advance;
  }
  if (align == Align::Center) x -= visibleWidth / 2;
  else if (align == Align::Right) x -= visibleWidth;
  uint16_t palette[16];
  for (int i = 0; i < 16; ++i) palette[i] = blend(front, back, i);
  while (*value && (!end || value < end)) {
    const auto *g = glyph(font, decode(value));
    if (g) { paint(c, x, y + font.ascent, *g, font, palette); x += g->advance; }
  }
  if (end && ellipsis) paint(c, x, y + font.ascent, *ellipsis, font, palette);
}
}
