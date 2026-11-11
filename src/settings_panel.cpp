#include "settings_panel.h"
#include "panel_ui.h"

namespace {
using namespace PanelUI;
bool inside(int x, int y, int left, int top, int width, int height) {
  return x >= left && x < left + width && y >= top && y < top + height;
}
String number(float value) {
  if (!isfinite(value)) return "--";
  return String(value, fabsf(value - roundf(value)) < 0.001f ? 0 : 1);
}
String label(const String &value) {
  if (value == "off") return "Wylaczone";
  if (value == "heat") return "Grzanie";
  if (value == "cool") return "Chlodzenie";
  if (value == "dry") return "Osuszanie";
  if (value == "fan_only") return "Wentylator";
  if (value == "auto") return "Auto";
  if (value == "low") return "Niski";
  if (value == "medium low") return "Srednio niski";
  if (value == "medium") return "Sredni";
  if (value == "medium high") return "Srednio wysoki";
  if (value == "high") return "Wysoki";
  if (value == "none") return "Normalny";
  if (value == "eco") return "Eco";
  if (value == "away") return "Poza domem";
  if (value == "boost") return "Turbo";
  if (value == "sleep") return "Sen";
  if (value == "default") return "Domyslny";
  if (value == "full_swing") return "Pelny ruch";
  if (value == "fixed_upper") return "Gora";
  if (value == "fixed_upper_middle") return "Gora / srodek";
  if (value == "fixed_middle") return "Srodek";
  if (value == "fixed_lower_middle") return "Dol / srodek";
  if (value == "fixed_lower") return "Dol";
  if (value == "swing_upper") return "Ruch: gora";
  if (value == "swing_upper_middle") return "Ruch: gora-srodek";
  if (value == "swing_middle") return "Ruch: srodek";
  if (value == "swing_lower_middle") return "Ruch: dol-srodek";
  if (value == "swing_lower") return "Ruch: dol";
  if (value == "left") return "Lewo";
  if (value == "left_center") return "Lewo / srodek";
  if (value == "center") return "Srodek";
  if (value == "right_center") return "Prawo / srodek";
  if (value == "right") return "Prawo";
  if (value == "MANUAL") return "Reczny";
  if (value == "PROGRAM") return "Program";
  String result = value;
  result.replace("_", " ");
  return result;
}
const char *fieldName(SettingField field) {
  switch (field) {
    case SettingField::Mode: return "TRYB PRACY";
    case SettingField::Fan: return "WENTYLATOR";
    case SettingField::Preset: return "PRESET";
    case SettingField::Swing: return "NAWIEW PIONOWY";
    case SettingField::Horizontal: return "NAWIEW POZIOMY";
    default: return "TRYB BOJLERA";
  }
}
void button(TFT_eSPI &c, int x, int y, int w, int h, const String &text,
            bool enabled, uint16_t accent = BLUE) {
  c.fillRoundRect(x, y, w, h, 7, enabled ? BUTTON : SURFACE);
  c.drawRoundRect(x, y, w, h, 7, enabled ? accent : BORDER);
  c.setTextColor(enabled ? TEXT : MUTED, enabled ? BUTTON : SURFACE);
  const int font = c.textWidth(text, 2) <= w - 8 ? 2 : 1;
  c.drawCentreString(text, x + w / 2, y + (h - c.fontHeight(font)) / 2, font);
}
float numeric(JsonVariantConst value) {
  return value.is<float>() ? value.as<float>() : NAN;
}
}

void OptionList::parse(JsonVariantConst options, const String &selected) {
  count = 0;
  current = selected;
  for (JsonVariantConst item : options.as<JsonArrayConst>()) {
    if (count == 24) break;
    if (item.is<const char *>()) values[count++] = item.as<String>();
  }
}
bool OptionList::contains(const String &value) const {
  for (uint8_t i = 0; i < count; ++i) if (values[i] == value) return true;
  return false;
}
void SettingsState::parse(const JsonDocument &json, bool boiler) {
  const String value = json["state"] | "";
  known = value.length() && value != "unknown" && value != "unavailable";
  updated = millis();
  const auto attrs = json["attributes"];
  features = attrs["supported_features"] | 0U;
  current = numeric(attrs["current_temperature"]);
  target = numeric(attrs["temperature"]);
  minimum = numeric(attrs["min_temp"]);
  maximum = numeric(attrs["max_temp"]);
  step = attrs["target_temp_step"] | 1.0f;
  if (boiler) options[5].parse(attrs["operation_list"], attrs["operation_mode"] | value);
  else {
    options[0].parse(attrs["hvac_modes"], value);
    options[1].parse(attrs["fan_modes"], attrs["fan_mode"] | "");
    options[2].parse(attrs["preset_modes"], attrs["preset_mode"] | "");
    options[3].parse(attrs["swing_modes"], attrs["swing_mode"] | "");
    options[4].parse(attrs["swing_horizontal_modes"], attrs["swing_horizontal_mode"] | "");
  }
}
bool SettingsState::fresh(bool connected) const {
  return connected && known && millis() - updated < 15000;
}
bool SettingsState::temperatureAvailable(bool connected) const {
  return fresh(connected) && (features & 1U) && isfinite(target) &&
         ControlRules::validRange(minimum, maximum, step);
}
bool SettingsState::fieldAvailable(SettingField field, bool connected) const {
  const uint32_t required[] = {0, 8, 16, 32, 512, 2};
  const uint8_t index = static_cast<uint8_t>(field);
  return fresh(connected) && options[index].count &&
         (!required[index] || (features & required[index]));
}
String SettingsState::signature(bool connected) const {
  String key = String(fresh(connected)) + ":" + features + ":" + number(current) + ":" +
               number(target) + ":" + number(minimum) + ":" + number(maximum) + ":" + number(step);
  for (const auto &list : options) {
    key += "|" + list.current;
    for (uint8_t i = 0; i < list.count; ++i) key += ":" + list.values[i];
  }
  return key;
}
void SettingsPanel::open(uint8_t deviceIndex) {
  device = deviceIndex;
  page = detailsPage();
  draft = state().target;
  dirty = false;
}
String SettingsPanel::signature(bool connected) const {
  return String(static_cast<int>(page)) + ":" + device + ":" + dirty + ":" + number(draft) + ":" +
         static_cast<int>(selectedField) + ":" + optionPage + ":" +
         heater.signature(connected) + ":" + climate.signature(connected) + ":" + boiler.signature(connected);
}
void SettingsPanel::draw(TFT_eSPI &c, bool connected, const String &notice) {
  SettingsState &s = state();
  if (!dirty) draft = s.target;
  const bool valid = s.fresh(connected), tempEnabled = s.temperatureAvailable(connected);
  const uint16_t accent = isBoiler() ? TEAL : (device == 0 ? AMBER : BLUE);
  c.fillRect(0, 0, WIDTH, HEIGHT, BG);
  c.setTextDatum(TL_DATUM);
  button(c, 8, 6, 58, 30, "< Wroc", true, accent);
  c.setTextColor(TEXT, BG);
  c.drawString(page == SettingsPage::Options ? fieldName(selectedField) :
               (isBoiler() ? "Bojler" : (device == 0 ? "Grzejnik" : "Klimatyzacja")), 77, 13, 2);
  if (page == SettingsPage::Options) {
    const auto &options = s.options[static_cast<uint8_t>(selectedField)];
    for (uint8_t row = 0; row < 4; ++row) {
      const uint8_t index = optionPage * 4 + row;
      if (index >= options.count) break;
      button(c, 8, 42 + row * 39, 304, 34,
             (options.current == options.values[index] ? "* " : "") + label(options.values[index]),
             s.fieldAvailable(selectedField, connected), accent);
    }
    button(c, 8, 201, 84, 22, "< Poprz.", optionPage > 0, accent);
    button(c, 228, 201, 84, 22, "Nast. >", (optionPage + 1) * 4 < options.count, accent);
    c.setTextColor(MUTED, BG);
    c.drawCentreString(String(optionPage + 1) + " / " + ((options.count + 3) / 4), 160, 205, 1);
  } else {
    button(c, 239, 6, 73, 30, "Zasilanie", valid, accent);
    c.setTextColor(MUTED, BG);
    c.drawString("Aktualna: " + (valid ? number(s.current) : String("--")), 12, 42, 2);
    c.drawRightString(dirty ? "Niezapisane" : "Stan z HA", 308, 45, 1);
    button(c, 8, 64, 46, 43, "-", tempEnabled, accent);
    button(c, 181, 64, 46, 43, "+", tempEnabled, accent);
    c.setTextColor(tempEnabled ? TEXT : MUTED, BG);
    c.drawCentreString(tempEnabled ? number(draft) : String("--"), 118, 63, 4);
    c.setTextColor(MUTED, BG);
    c.drawCentreString("TEMP. ZADANA", 118, 94, 1);
    button(c, 239, 64, 73, 43, "Zapisz", tempEnabled && dirty &&
           ControlRules::canSave(draft, s.minimum, s.maximum, s.step), accent);
    if (isBoiler()) {
      c.setTextColor(MUTED, BG);
      c.drawString("Zakres z HA: " + number(s.minimum) + " - " + number(s.maximum), 12, 117, 2);
      c.drawString("Krok: " + number(s.step), 12, 139, 2);
      const auto &list = s.options[5];
      button(c, 8, 170, 304, 42, "Tryb: " + label(list.current),
             s.fieldAvailable(SettingField::BoilerMode, connected), accent);
    } else {
      uint8_t fields[5];
      const uint8_t count = visibleFields(fields);
      for (uint8_t slot = 0; slot < count; ++slot) {
        const uint8_t i = fields[slot];
        const int x = 8 + (slot % 2) * 156, y = 116 + (slot / 2) * 34;
        const int width = count == 1 ? 304 : 148;
        const bool enabled = s.fieldAvailable(static_cast<SettingField>(i), connected);
        c.fillRoundRect(x, y, width, 30, 6, SURFACE);
        c.drawRoundRect(x, y, width, 30, 6, enabled ? BORDER : SURFACE);
        c.setTextColor(MUTED, SURFACE);
        c.drawString(fieldName(static_cast<SettingField>(i)), x + 6, y + 3, 1);
        c.setTextColor(enabled ? TEXT : MUTED, SURFACE);
        const String value = enabled ? label(s.options[i].current) : String("Niedostepne");
        c.drawString(value.substring(0, 22) + (enabled ? " >" : ""), x + 6, y + 16, 1);
      }
      c.setTextColor(MUTED, BG);
      if (device == 0) {
        c.drawString("Zakres: " + number(s.minimum) + " - " + number(s.maximum), 12, 160, 2);
        c.drawString("Krok: " + number(s.step), 12, 184, 2);
      } else c.drawCentreString(valid ? "Odczyt z HA" : "Brak danych HA", 238, 193, 1);
    }
  }
  c.setTextColor(MUTED, BG);
  String footer = notice;
  if (!footer.length() && dirty && tempEnabled &&
      !ControlRules::canSave(draft, s.minimum, s.maximum, s.step))
    footer = "Nastawa poza aktualnym zakresem lub krokiem HA";
  if (!footer.length()) footer = !valid ? "Brak aktualnych danych. Sterowanie zablokowane." :
      (page == SettingsPage::Options ? "Dotknij opcji, aby ja zastosowac" : "Zmiana temperatury wymaga przycisku Zapisz");
  c.drawString(footer.substring(0, 49), 12, 229, 1);
}
SettingsAction SettingsPanel::tap(int x, int y, bool connected) {
  SettingsAction action;
  action.device = device;
  SettingsState &s = state();
  if (inside(x, y, 8, 6, 58, 30)) {
    if (page == SettingsPage::Options) page = detailsPage();
    else { page = SettingsPage::Home; action.type = SettingsActionType::BackHome; dirty = false; }
    return action;
  }
  if (page == SettingsPage::Options) {
    const auto &options = s.options[static_cast<uint8_t>(selectedField)];
    if (inside(x, y, 8, 201, 84, 22) && optionPage) --optionPage;
    else if (inside(x, y, 228, 201, 84, 22) && (optionPage + 1) * 4 < options.count) ++optionPage;
    else if (s.fieldAvailable(selectedField, connected)) {
      for (uint8_t row = 0; row < 4; ++row) {
        const uint8_t index = optionPage * 4 + row;
        if (index < options.count && inside(x, y, 8, 42 + row * 39, 304, 34)) {
          action.type = SettingsActionType::SelectOption;
          action.field = selectedField;
          action.option = options.values[index];
        }
      }
    }
    return action;
  }
  if (inside(x, y, 239, 6, 73, 30) && s.fresh(connected)) {
    action.type = SettingsActionType::Toggle;
    return action;
  }
  if (s.temperatureAvailable(connected)) {
    if (!dirty) draft = s.target;
    if (inside(x, y, 8, 64, 46, 43) || inside(x, y, 181, 64, 46, 43)) {
      draft = ControlRules::adjust(draft, x < 54 ? -1 : 1, s.minimum, s.maximum, s.step);
      dirty = fabsf(draft - s.target) > 0.001f;
    } else if (inside(x, y, 239, 64, 73, 43) && dirty &&
               ControlRules::canSave(draft, s.minimum, s.maximum, s.step)) {
      action.type = SettingsActionType::SaveTemperature;
      action.temperature = draft;
    }
  }
  uint8_t fields[5];
  const uint8_t count = isBoiler() ? 1 : visibleFields(fields);
  if (isBoiler()) fields[0] = 5;
  for (uint8_t slot = 0; slot < count; ++slot) {
    const uint8_t i = fields[slot];
    const bool hit = isBoiler() ? inside(x, y, 8, 170, 304, 42) :
        inside(x, y, 8 + (slot % 2) * 156, 116 + (slot / 2) * 34, count == 1 ? 304 : 148, 30);
    if (hit && s.fieldAvailable(static_cast<SettingField>(i), connected)) {
      selectedField = static_cast<SettingField>(i);
      page = SettingsPage::Options;
      optionPage = 0;
      for (uint8_t j = 0; j < s.options[i].count; ++j)
        if (s.options[i].values[j] == s.options[i].current) { optionPage = j / 4; break; }
      break;
    }
  }
  return action;
}
void SettingsPanel::saved(bool temperatureSaved) {
  // Po HTTP 200 widok nadal pokazuje odczyt z HA, nie wyslana wartosc.
  if (page == SettingsPage::Options) page = detailsPage();
  if (temperatureSaved) dirty = false;
}

uint8_t SettingsPanel::visibleFields(uint8_t *fields) const {
  const auto &s = device == 0 ? heater : climate;
  const uint32_t required[] = {0, 8, 16, 32, 512};
  uint8_t count = 0;
  for (uint8_t i = 0; i < 5; ++i)
    if (i == 0 || (s.options[i].count && (s.features & required[i]))) fields[count++] = i;
  return count;
}
