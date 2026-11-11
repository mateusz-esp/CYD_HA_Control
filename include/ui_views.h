#pragma once
#include "panel_ui.h"
#include "ui_font.h"

namespace Views {
struct DeviceTile {
  const char *name, *status, *current, *target;
  bool active, available;
};
struct SettingTile {
  const char *name, *value;
  bool available;
};
struct Detail {
  uint8_t device;
  bool available, temperatureAvailable, dirty, saveAvailable;
  const char *target, *current, *range, *step;
  SettingTile fields[5];
  uint8_t count;
};
struct Option {
  const char *name;
  bool selected;
};
void home(TFT_eSPI &canvas, const DeviceTile *devices, bool wifi, const char *notice);
void detail(TFT_eSPI &canvas, const Detail &state, const char *notice);
void options(TFT_eSPI &canvas, uint8_t device, const char *title, const Option *rows,
             uint8_t count, uint8_t page, uint8_t pages, bool available, const char *notice);
}
