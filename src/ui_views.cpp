#include "ui_views.h"
#include <stdio.h>

namespace Views {
namespace {
using namespace PanelUI;
using namespace Typography;
void pill(TFT_eSPI &c, Rect r, const char *value, uint16_t foreground, uint16_t background,
          const Font &font = Body) {
  c.fillRoundRect(r.x, r.y, r.w, r.h, r.h / 2, background);
  text(c, r.x + r.w / 2, r.y + (r.h - font.height) / 2, value, font, foreground, background,
       r.w - 12, Align::Center);
}
void footer(TFT_eSPI &c, const char *notice, const char *hint) {
  c.drawFastHLine(12, 220, 296, BORDER);
  text(c, 12, 225, notice && *notice ? notice : hint, Small, MUTED, BG, 296);
}
void back(TFT_eSPI &c) {
  c.fillRoundRect(BACK.x, BACK.y, BACK.w, BACK.h, 10, SURFACE);
  c.drawLine(28, 18, 22, 24, TEXT);
  c.drawLine(22, 24, 28, 30, TEXT);
}
void stepper(TFT_eSPI &c, Rect r, bool plus, bool enabled, uint16_t accent) {
  const uint16_t bg = enabled ? BUTTON : BG;
  c.fillRoundRect(r.x, r.y, r.w, r.h, 10, bg);
  const int x = r.x + r.w / 2, y = r.y + r.h / 2;
  const uint16_t color = enabled ? accent : MUTED;
  c.fillRect(x - 5, y - 1, 11, 2, color);
  if (plus) c.fillRect(x - 1, y - 5, 2, 11, color);
}
}
void home(TFT_eSPI &c, const DeviceTile *devices, bool wifi, const char *notice) {
  using namespace PanelUI;
  using namespace Typography;
  c.fillRect(0, 0, WIDTH, HEIGHT, BG);
  text(c, 12, 5, "TWÓJ DOM", Small, MUTED, BG, 200);
  text(c, 12, 20, "Komfort na co dzień", Title, TEXT, BG, 225);
  c.fillRoundRect(248, 12, 60, 26, 13, SURFACE);
  c.fillCircle(259, 25, 3, wifi ? TEAL : AMBER);
  text(c, 269, 18, wifi ? "Wi-Fi" : "Offline", Small, wifi ? TEAL : MUTED, SURFACE, 35);
  for (uint8_t i = 0; i < 3; ++i) {
    const auto &d = devices[i];
    const int x = cardX(i);
    const uint16_t accent = ACCENTS[i];
    c.fillRoundRect(x, CARD_Y + 2, CARD_W, CARD_H, 13, SHADOW);
    c.fillRoundRect(x, CARD_Y, CARD_W, CARD_H, 13, SURFACE);
    if (d.active) c.drawRoundRect(x, CARD_Y, CARD_W, CARD_H, 13, BORDER);
    c.fillRoundRect(x + 10, 72, 30, 30, 9, ICON_BG[i]);
    icon(c, i, x + 25, 87, accent);
    c.fillCircle(x + 80, 82, 3, d.active ? accent : BORDER);
    text(c, x + 10, 106, d.name, Body, TEXT, SURFACE, CARD_W - 20);
    text(c, x + 10, 122, d.status, Small, d.active ? accent : MUTED, SURFACE, CARD_W - 20);
    text(c, x + 10, 132, d.current, Number, d.available ? TEXT : MUTED, SURFACE, CARD_W - 16);
    text(c, x + 10, 162, d.target, Small, MUTED, SURFACE, CARD_W - 20);
    const Rect r = homePower(i);
    const uint16_t bg = d.available && d.active ? ACTIVE_BG[i] : BUTTON;
    c.fillRoundRect(r.x, r.y, r.w, r.h, 9, bg);
    power(c, r.x + 13, r.y + 13, d.available ? (d.active ? accent : TEXT) : MUTED, bg);
    text(c, r.x + 24, r.y + 6, d.available ? (d.active ? "Wyłącz" : "Włącz") : "Offline",
         Small, d.available ? (d.active ? accent : TEXT) : MUTED, bg, r.w - 28);
  }
  footer(c, notice, "Dotknij kafelka, aby otworzyć ustawienia");
}
void detail(TFT_eSPI &c, const Detail &s, const char *notice) {
  using namespace PanelUI;
  using namespace Typography;
  const uint16_t accent = ACCENTS[s.device];
  c.fillRect(0, 0, WIDTH, HEIGHT, BG);
  back(c);
  text(c, 56, 9, s.device == 0 ? "Grzejnik" : (s.device == 1 ? "Klimatyzacja" : "Bojler"),
       Title, TEXT, BG, 176);
  pill(c, POWER, "Zasilanie", s.available ? accent : MUTED, SURFACE, Small);

  c.fillRoundRect(8, 48, 304, 76, 13, SURFACE);
  text(c, 20, 54, "Temperatura zadana", Small, MUTED, SURFACE, 130);
  text(c, 300, 54, s.current, Small, MUTED, SURFACE, 140, Align::Right);
  text(c, 20, 73, s.target, Number, s.temperatureAvailable ? TEXT : MUTED, SURFACE, 125);
  stepper(c, MINUS, false, s.temperatureAvailable, accent);
  stepper(c, PLUS, true, s.temperatureAvailable, accent);
  pill(c, SAVE, "Zapisz", s.saveAvailable ? SURFACE : MUTED,
       s.saveAvailable ? accent : BUTTON, Body);
  if (s.dirty) c.fillCircle(137, 111, 2, accent);

  for (uint8_t i = 0; i < s.count; ++i) {
    const Rect r = fieldRect(i, s.count);
    const auto &field = s.fields[i];
    c.fillRoundRect(r.x, r.y, r.w, r.h, 8, SURFACE);
    const bool large = s.count == 1;
    text(c, r.x + 10, r.y + (large ? 3 : 0), field.name, Small, MUTED, SURFACE, r.w - 30);
    text(c, r.x + 10, r.y + (large ? 18 : 12), field.value, Body,
         field.available ? TEXT : MUTED, SURFACE, r.w - 28);
    if (field.available) {
      c.drawLine(r.x + r.w - 15, r.y + r.h / 2 - 3, r.x + r.w - 12, r.y + r.h / 2, MUTED);
      c.drawLine(r.x + r.w - 12, r.y + r.h / 2, r.x + r.w - 15, r.y + r.h / 2 + 3, MUTED);
    }
  }
  if (s.count <= 2) {
    text(c, 20, 183, s.range, Body, MUTED, BG, 280);
    text(c, 20, 201, s.step, Small, MUTED, BG, 280);
  }
  footer(c, notice, !s.available ? "Brak połączenia z urządzeniem" :
         (s.dirty ? "Nowa temperatura czeka na zapisanie" : "Ustawienia zsynchronizowane z domem"));
}
void options(TFT_eSPI &c, uint8_t device, const char *title, const Option *rows,
             uint8_t count, uint8_t page, uint8_t pages, bool available, const char *notice) {
  using namespace PanelUI;
  using namespace Typography;
  const uint16_t accent = ACCENTS[device];
  c.fillRect(0, 0, WIDTH, HEIGHT, BG);
  back(c);
  text(c, 56, 9, title, Title, TEXT, BG, 252);
  for (uint8_t i = 0; i < count; ++i) {
    const Rect r = optionRow(i);
    const uint16_t bg = rows[i].selected ? ACTIVE_BG[device] : SURFACE;
    c.fillRoundRect(r.x, r.y, r.w, r.h, 10, bg);
    text(c, r.x + 12, r.y + 8, rows[i].name, Label, available ? TEXT : MUTED, bg, 250);
    c.drawCircle(290, r.y + 17, 6, rows[i].selected ? accent : BORDER);
    if (rows[i].selected) c.fillCircle(290, r.y + 17, 3, accent);
  }
  pill(c, PREVIOUS, "Wstecz", page ? TEXT : MUTED, page ? SURFACE : BG, Small);
  pill(c, NEXT, "Dalej", page + 1 < pages ? TEXT : MUTED, page + 1 < pages ? SURFACE : BG, Small);
  char pagination[16];
  snprintf(pagination, sizeof(pagination), "%u / %u", page + 1, pages);
  text(c, 160, 206, pagination, Small, MUTED, BG, 100, Align::Center);
  text(c, 12, 228, notice && *notice ? notice : "Wybierz opcję, aby zastosować zmianę", Small, MUTED, BG, 296);
}
}
