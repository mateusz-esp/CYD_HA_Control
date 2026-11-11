#include "settings_panel.h"
#include "panel_ui.h"
#include "ui_views.h"

namespace {
using namespace PanelUI;
String number(float value) {
  if (!isfinite(value)) return "--";
  return String(value, fabsf(value - roundf(value)) < 0.001f ? 0 : 1);
}
String label(const String &value) {
  if (value == "off") return "Wyłączone";
  if (value == "heat") return "Grzanie";
  if (value == "cool") return "Chłodzenie";
  if (value == "dry") return "Osuszanie";
  if (value == "fan_only") return "Wentylator";
  if (value == "auto") return "Auto";
  if (value == "low") return "Niski";
  if (value == "medium low") return "Średnio niski";
  if (value == "medium") return "Średni";
  if (value == "medium high") return "Średnio wysoki";
  if (value == "high") return "Wysoki";
  if (value == "none") return "Normalny";
  if (value == "eco") return "Eco";
  if (value == "away") return "Poza domem";
  if (value == "boost") return "Turbo";
  if (value == "sleep") return "Sen";
  if (value == "default") return "Domyślny";
  if (value == "full_swing") return "Pełny ruch";
  if (value == "fixed_upper") return "Góra";
  if (value == "fixed_upper_middle") return "Góra / środek";
  if (value == "fixed_middle") return "Środek";
  if (value == "fixed_lower_middle") return "Dół / środek";
  if (value == "fixed_lower") return "Dół";
  if (value == "swing_upper") return "Ruch: góra";
  if (value == "swing_upper_middle") return "Ruch: góra-środek";
  if (value == "swing_middle") return "Ruch: środek";
  if (value == "swing_lower_middle") return "Ruch: dół-środek";
  if (value == "swing_lower") return "Ruch: dół";
  if (value == "left") return "Lewo";
  if (value == "left_center") return "Lewo / środek";
  if (value == "center") return "Środek";
  if (value == "right_center") return "Prawo / środek";
  if (value == "right") return "Prawo";
  if (value == "MANUAL") return "Ręczny";
  if (value == "PROGRAM") return "Program";
  String result = value;
  result.replace("_", " ");
  return result;
}
const char *fieldName(SettingField field) {
  switch (field) {
    case SettingField::Mode: return "Tryb pracy";
    case SettingField::Fan: return "Wentylator";
    case SettingField::Preset: return "Profil";
    case SettingField::Swing: return "Nawiew pionowy";
    case SettingField::Horizontal: return "Nawiew poziomy";
    default: return "Tryb bojlera";
  }
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
  if (page == SettingsPage::Options) {
    const auto &list = s.options[static_cast<uint8_t>(selectedField)];
    const uint8_t pages = list.count ? (list.count + 3) / 4 : 1;
    if (optionPage >= pages) optionPage = pages - 1;
    String names[4];
    Views::Option rows[4];
    uint8_t count = 0;
    for (uint8_t row = 0; row < 4 && optionPage * 4 + row < list.count; ++row) {
      const uint8_t index = optionPage * 4 + row;
      names[row] = label(list.values[index]);
      rows[row] = {names[row].c_str(), list.current == list.values[index]};
      ++count;
    }
    Views::options(c, device, fieldName(selectedField), rows, count, optionPage, pages,
                   s.fieldAvailable(selectedField, connected), notice.c_str());
    return;
  }
  const String target = tempEnabled ? number(draft) + "°" : String("--");
  const String current = valid && isfinite(s.current) ?
      String(isBoiler() ? "Woda: " : "Teraz: ") + number(s.current) + "°" : "Brak odczytu";
  const String range = "Zakres: " + number(s.minimum) + " - " + number(s.maximum) + "°";
  const String step = "Krok regulacji: " + number(s.step) + "°";
  Views::Detail view = {};
  view.device = device;
  view.available = valid;
  view.temperatureAvailable = tempEnabled;
  view.dirty = dirty;
  view.saveAvailable = tempEnabled && dirty && ControlRules::canSave(draft, s.minimum, s.maximum, s.step);
  view.target = target.c_str();
  view.current = current.c_str();
  view.range = range.c_str();
  view.step = step.c_str();
  uint8_t fields[5];
  view.count = isBoiler() ? 1 : visibleFields(fields);
  if (isBoiler()) fields[0] = 5;
  String values[5];
  for (uint8_t slot = 0; slot < view.count; ++slot) {
    const uint8_t field = fields[slot];
    const bool enabled = s.fieldAvailable(static_cast<SettingField>(field), connected);
    values[slot] = enabled ? label(s.options[field].current) : String("Niedostępne");
    view.fields[slot] = {fieldName(static_cast<SettingField>(field)), values[slot].c_str(), enabled};
  }
  String footer = notice;
  if (!footer.length() && dirty && tempEnabled && !view.saveAvailable)
    footer = "Nastawa poza aktualnym zakresem temperatury";
  Views::detail(c, view, footer.c_str());
}

SettingsAction SettingsPanel::tap(int x, int y, bool connected) {
  SettingsAction action;
  action.device = device;
  SettingsState &s = state();
  if (PanelUI::BACK.contains(x, y)) {
    if (page == SettingsPage::Options) page = detailsPage();
    else { page = SettingsPage::Home; action.type = SettingsActionType::BackHome; dirty = false; }
    return action;
  }
  if (page == SettingsPage::Options) {
    const auto &options = s.options[static_cast<uint8_t>(selectedField)];
    if (PanelUI::PREVIOUS.contains(x, y) && optionPage) --optionPage;
    else if (PanelUI::NEXT.contains(x, y) && (optionPage + 1) * 4 < options.count) ++optionPage;
    else if (s.fieldAvailable(selectedField, connected)) {
      for (uint8_t row = 0; row < 4; ++row) {
        const uint8_t index = optionPage * 4 + row;
        if (index < options.count && PanelUI::optionRow(row).contains(x, y)) {
          action.type = SettingsActionType::SelectOption;
          action.field = selectedField;
          action.option = options.values[index];
        }
      }
    }
    return action;
  }
  if (PanelUI::POWER.contains(x, y) && s.fresh(connected)) {
    action.type = SettingsActionType::Toggle;
    return action;
  }
  if (s.temperatureAvailable(connected)) {
    if (!dirty) draft = s.target;
    if (PanelUI::MINUS.contains(x, y) || PanelUI::PLUS.contains(x, y)) {
      draft = ControlRules::adjust(draft, PanelUI::MINUS.contains(x, y) ? -1 : 1, s.minimum, s.maximum, s.step);
      dirty = fabsf(draft - s.target) > 0.001f;
    } else if (PanelUI::SAVE.contains(x, y) && dirty &&
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
    const bool hit = PanelUI::fieldRect(slot, count).contains(x, y);
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
