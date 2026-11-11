#pragma once
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include "control_rules.h"

enum class SettingsPage { Home, Heater, Climate, Boiler, Options };
enum class SettingField : uint8_t { Mode, Fan, Preset, Swing, Horizontal, BoilerMode };
struct OptionList {
  String values[24];
  uint8_t count = 0;
  String current;
  void parse(JsonVariantConst options, const String &selected);
  bool contains(const String &value) const;
};
struct SettingsState {
  bool known = false;
  uint32_t updated = 0, features = 0;
  float current = NAN, target = NAN, minimum = NAN, maximum = NAN, step = NAN;
  OptionList options[6];
  void parse(const JsonDocument &json, bool boiler);
  bool fresh(bool connected) const;
  bool temperatureAvailable(bool connected) const;
  bool fieldAvailable(SettingField field, bool connected) const;
  String signature(bool connected) const;
};
enum class SettingsActionType { None, BackHome, Toggle, SaveTemperature, SelectOption };
struct SettingsAction {
  SettingsActionType type = SettingsActionType::None;
  uint8_t device = 0; // 0: grzejnik, 1: klimatyzacja, 2: bojler.
  SettingField field = SettingField::Mode;
  float temperature = NAN;
  String option;
};
class SettingsPanel {
 public:
  SettingsState heater, climate, boiler;
  SettingsPage page = SettingsPage::Home;
  void open(uint8_t deviceIndex);
  SettingsState &forDevice(uint8_t index) { return index == 2 ? boiler : (index == 0 ? heater : climate); }
  void draw(TFT_eSPI &screen, bool connected, const String &notice);
  SettingsAction tap(int x, int y, bool connected);
  void saved(bool temperatureSaved);
  String signature(bool connected) const;
 private:
  uint8_t device = 0;
  bool dirty = false;
  float draft = NAN;
  SettingField selectedField = SettingField::Mode;
  uint8_t optionPage = 0;
  bool isBoiler() const { return device == 2; }
  SettingsPage detailsPage() const {
    return isBoiler() ? SettingsPage::Boiler : (device == 0 ? SettingsPage::Heater : SettingsPage::Climate);
  }
  SettingsState &state() { return forDevice(device); }
  uint8_t visibleFields(uint8_t *fields) const;
};
