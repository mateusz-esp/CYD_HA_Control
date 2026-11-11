#include <Arduino.h>
#include <math.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <XPT2046_Touchscreen.h>
#include "panel_ui.h"
#include "settings_panel.h"
#include "partial_display.h"
#include "ui_views.h"

#if __has_include("config.h")
#include "config.h"
constexpr bool configured = true;
#else
#include "config.example.h"
constexpr bool configured = false;
#endif

static_assert(sizeof(Config::DEVICES) / sizeof(Config::DEVICES[0]) == 3,
              "Panel wymaga trzech urzadzen");
static_assert(Config::TOUCH_X_MIN != Config::TOUCH_X_MAX &&
              Config::TOUCH_Y_MIN != Config::TOUCH_Y_MAX, "Bledna kalibracja");
TFT_eSPI screen;
PartialDisplay displayUpdates(screen);
SPIClass touchSPI(VSPI);
XPT2046_Touchscreen touch(33, 36);
constexpr uint32_t POLL_MS = 5000, STALE_MS = 15000;
struct DeviceState {
  String value;
  String temperature;
  bool known = false, enabled = false;
  uint32_t updated = 0;
};
DeviceState states[3];
struct SensorState {
  String value, unit;
  bool known = false;
  uint32_t updated = 0;
  bool fresh() const {
    return known && millis() - updated < STALE_MS && WiFi.status() == WL_CONNECTED;
  }
};
SensorState boilerHeating, boilerTarget;
SettingsPanel settings;
String notice;
uint32_t noticeUntil = 0, lastPoll = 0, lastWifiTry = 0;
uint8_t pollIndex = 0;
uint32_t releaseSince = 0;
bool polling = true, wasConnected = false, pressed = false;

bool fresh(uint8_t i) {
  return states[i].known && millis() - states[i].updated < STALE_MS &&
         WiFi.status() == WL_CONNECTED;
}

const char *climateMode(const String &value) {
  if (value == "heat") return "Grzanie";
  if (value == "cool") return "Chłodzenie";
  if (value == "dry") return "Osuszanie";
  if (value == "fan_only") return "Wentylator";
  if (value == "auto" || value == "heat_cool") return "Auto";
  return "Włączone";
}

void drawScene(TFT_eSPI &canvas) {
  const bool connected = WiFi.status() == WL_CONNECTED;
  if (settings.page != SettingsPage::Home) {
    settings.draw(canvas, connected, notice);
    return;
  }
  String current[3], target[3];
  Views::DeviceTile cards[3];
  for (uint8_t i = 0; i < 3; ++i) {
    const bool valid = fresh(i), active = valid && states[i].enabled;
    const auto &snapshot = settings.forDevice(i);
    const bool temperatures = snapshot.fresh(connected);
    current[i] = temperatures && isfinite(snapshot.current) ?
        String(snapshot.current, snapshot.current == truncf(snapshot.current) ? 0 : 1) + "°" : "--";
    if (temperatures && isfinite(snapshot.target))
      target[i] = "Cel " + String(snapshot.target, snapshot.target == truncf(snapshot.target) ? 0 : 1) + "°";
    else if (i == 2 && boilerTarget.fresh()) target[i] = "Cel " + boilerTarget.value + "°";
    else target[i] = "Brak nastawy";
    const char *status = !strlen(Config::DEVICES[i].entity) ? "Brak konfiguracji" :
        (!valid ? "Niedostępne" : (active ? "Włączone" : (i == 1 ? "Wyłączona" : "Wyłączony")));
    if (i < 2 && active) status = climateMode(states[i].value);
    if (i == 2 && active && boilerHeating.fresh()) status = boilerHeating.value == "on" ? "Grzeje" : "Nie grzeje";
    cards[i] = {Config::DEVICES[i].label, status, current[i].c_str(), target[i].c_str(), active, valid};
  }
  const String footer = !configured && !notice.length() ? "Skonfiguruj połączenie z Home Assistant" : notice;
  Views::home(canvas, cards, connected, footer.c_str());
}

void draw() {
  const bool connected = WiFi.status() == WL_CONNECTED;
  String key = String(connected) + "|" + notice;
  for (uint8_t i = 0; i < 3; ++i)
    key += String("|") + fresh(i) + ":" + states[i].enabled + ":" + states[i].value + ":" + states[i].temperature;
  key += String("|") + boilerHeating.fresh() + ":" + boilerHeating.value;
  key += String("|") + boilerTarget.fresh() + ":" + boilerTarget.value + ":" + boilerTarget.unit;
  key += "|" + settings.signature(connected);
  static String previous;
  if (key == previous) return;
  previous = key;
  displayUpdates.present([](TFT_eSPI &canvas) { drawScene(canvas); });
}

void message(const String &text) {
  notice = text;
  noticeUntil = millis() + 6000;
  draw();
}

int request(const String &path, String &response, const String *body = nullptr) {
  WiFiClient client;
  HTTPClient http;
  if (!http.begin(client, String(Config::HA_URL) + path)) return -1;
  http.setConnectTimeout(1500);
  http.setTimeout(2000);
  http.addHeader("Authorization", String("Bearer ") + Config::HA_TOKEN);
  http.addHeader("Content-Type", "application/json");
  const int code = body ? http.POST(*body) : http.GET();
  if (code == 200 && !body) response = http.getString();
  http.end();
  return code;
}

void apiError(int code) {
  Serial.printf("HA: blad HTTP %d\n", code);
  if (code == 401 || code == 403) message("HA: sprawdz token i uprawnienia");
  else if (code == 404) message("HA: nie znaleziono encji/uslugi");
  else message(String("Blad HA / HTTP: ") + code);
}

void readState(uint8_t i) {
  if (!strlen(Config::DEVICES[i].entity)) return;
  String response;
  const int code = request(String("/api/states/") + Config::DEVICES[i].entity, response);
  JsonDocument json;
  if (code != 200) {
    states[i].known = false;
    if (i < 2) settings.forDevice(i).known = false;
    apiError(code);
    return;
  }
  if (deserializeJson(json, response) || !json["state"].is<const char *>()) {
    states[i].known = false;
    if (i < 2) settings.forDevice(i).known = false;
    message("HA: niepoprawna odpowiedz");
    return;
  }
  DeviceState &s = states[i];
  const bool changed = !s.known || s.value != json["state"].as<String>();
  s.value = json["state"].as<String>();
  s.known = s.value.length() && s.value != "unknown" && s.value != "unavailable";
  s.temperature = "";
  if (i < 2) {
    settings.forDevice(i).parse(json, false);
    const auto attributes = json["attributes"];
    if (attributes["current_temperature"].is<float>())
      s.temperature = String(attributes["current_temperature"].as<float>(), 1) + " / ";
    if (attributes["temperature"].is<float>())
      s.temperature += String(attributes["temperature"].as<float>(), 1);
    else s.temperature += "--";
    // Jednostka wynika z konfiguracji temperatury HA, nie jest wymuszana na C.
  }
  // climate: heat/cool/auto oznacza wlaczony tryb, nie prace sprezarki.
  s.enabled = s.value != "off";
  s.updated = millis();
  if (changed) Serial.printf("HA: %s = %s\n", Config::DEVICES[i].entity, s.value.c_str());
  draw();
}

void readSensor(const char *entity, SensorState &sensor, bool binary = false) {
  String response;
  const int code = request(String("/api/states/") + entity, response);
  JsonDocument json;
  if (code != 200) {
    sensor.known = false;
    apiError(code);
    return;
  }
  if (deserializeJson(json, response) || !json["state"].is<const char *>()) {
    sensor.known = false;
    message("HA: niepoprawna odpowiedz czujnika");
    return;
  }
  const String oldValue = sensor.value;
  const bool wasKnown = sensor.known;
  sensor.value = json["state"].as<String>();
  sensor.unit = json["attributes"]["unit_of_measurement"] | "";
  sensor.known = sensor.value.length() && sensor.value != "unknown" && sensor.value != "unavailable";
  if (binary) sensor.known = sensor.value == "on" || sensor.value == "off";
  else if (sensor.known) {
    char *end;
    const float value = strtof(sensor.value.c_str(), &end);
    sensor.known = *end == '\0' && isfinite(value);
    if (sensor.known) sensor.value = String(value, value == truncf(value) ? 0 : 1);
  }
  sensor.updated = millis();
  if (!wasKnown || oldValue != sensor.value)
    Serial.printf("HA: %s = %s\n", entity, sensor.value.c_str());
  draw();
}

void readBoilerSettings() {
  String response;
  const int code = request(String("/api/states/") + Config::BOILER_CONTROL_ENTITY, response);
  JsonDocument json;
  if (code != 200) {
    settings.boiler.known = false;
    apiError(code);
    return;
  }
  if (deserializeJson(json, response) || !json["state"].is<const char *>()) {
    settings.boiler.known = false;
    message("HA: bledna odpowiedz ustawien bojlera");
    return;
  }
  settings.boiler.parse(json, true);
  draw();
}

void toggle(uint8_t i) {
  if (!fresh(i)) {
    message("Brak aktualnego odczytu urządzenia");
    return;
  }
  const auto &d = Config::DEVICES[i];
  const bool turnOn = !states[i].enabled;
  JsonDocument data;
  if (deserializeJson(data, turnOn ? d.onData : d.offData) || !data.is<JsonObject>()) {
    message("Bledny JSON uslugi w config.h");
    return;
  }
  data["entity_id"] = d.entity;
  String body, response;
  serializeJson(data, body);
  message("Wysyłanie polecenia…");
  const int code = request(String("/api/services/") + d.domain + "/" +
                           (turnOn ? d.onService : d.offService), response, &body);
  // Stan potwierdza HA; brak automatycznego ponawiania polecen.
  if (code != 200) {
    states[i].known = false;
    apiError(code);
  }
  else {
    message("Wysłano. Oczekiwanie na urządzenie…");
    readState(i);
  }
  polling = true;
  pollIndex = 0;
}

void applySetting(const SettingsAction &action) {
  const bool connected = WiFi.status() == WL_CONNECTED;
  if (action.type == SettingsActionType::Toggle) {
    toggle(action.device);
    return;
  }
  if (action.type != SettingsActionType::SaveTemperature &&
      action.type != SettingsActionType::SelectOption) return;
  const bool boiler = action.device == 2;
  auto &state = settings.forDevice(action.device);
  JsonDocument data;
  const String domain = boiler ? "water_heater" : "climate";
  data["entity_id"] = boiler ? Config::BOILER_CONTROL_ENTITY : Config::DEVICES[action.device].entity;
  String service;
  if (action.type == SettingsActionType::SaveTemperature) {
    if (!state.temperatureAvailable(connected) ||
        !ControlRules::canSave(action.temperature, state.minimum, state.maximum, state.step)) {
      message("Sprawdź połączenie i zakres temperatury");
      return;
    }
    service = "set_temperature";
    data["temperature"] = action.temperature;
  } else {
    const uint8_t index = static_cast<uint8_t>(action.field);
    if (!state.fieldAvailable(action.field, connected) || !state.options[index].contains(action.option)) {
      message("Ta opcja jest teraz niedostępna");
      return;
    }
    const char *services[] = {"set_hvac_mode", "set_fan_mode", "set_preset_mode",
                             "set_swing_mode", "set_swing_horizontal_mode", "set_operation_mode"};
    const char *fields[] = {"hvac_mode", "fan_mode", "preset_mode", "swing_mode",
                           "swing_horizontal_mode", "operation_mode"};
    service = services[index];
    data[fields[index]] = action.option;
  }
  String body, response;
  serializeJson(data, body);
  message("Zapisywanie ustawienia…");
  const int code = request("/api/services/" + domain + "/" + service, response, &body);
  // Nie ponawiaj automatycznie polecenia, nawet przy niepewnym wyniku HTTP.
  if (code != 200) {
    state.known = false;
    apiError(code);
  }
  else {
    settings.saved(action.type == SettingsActionType::SaveTemperature);
    message("Wysłano. Oczekiwanie na potwierdzenie…");
  }
  if (boiler) readBoilerSettings();
  else readState(action.device);
  polling = true;
  pollIndex = 0;
}

void handleTap(int x, int y) {
  if (settings.page != SettingsPage::Home) {
    applySetting(settings.tap(x, y, WiFi.status() == WL_CONNECTED));
    draw();
    return;
  }
  for (uint8_t i = 0; i < 3; ++i) {
    if (!PanelUI::hit(i, x, y)) continue;
    // Ustawienia i zasilanie maja oddzielne obszary dotyku.
    if (PanelUI::homePower(i).contains(x, y)) toggle(i);
    else {
      settings.open(i);
      polling = true;
      pollIndex = 0;
      draw();
    }
    break;
  }
}

void setup() {
  Serial.begin(115200);
  // RGB LED wylaczone, karta SD nieaktywna.
  for (int pin : {4, 16, 17, 5}) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH);
  }
  screen.init();
  screen.setRotation(PanelUI::ROTATION);
  Serial.printf("CYD-HA LCD2 / build %s %s\n", __DATE__, __TIME__);
  Serial.printf("LCD: rotation=%u, %dx%d, SPI=%u Hz\n",
                PanelUI::ROTATION, screen.width(), screen.height(), SPI_FREQUENCY);
  // Odczyt diagnostyczny: nie wszystkie panele udostepniaja rejestry po MISO.
  Serial.printf("LCD: ID=%02X %02X %02X, MADCTL=%02X (odczyt moze byc niedostepny)\n",
                screen.readcommand8(0xD3, 1), screen.readcommand8(0xD3, 2),
                screen.readcommand8(0xD3, 3), screen.readcommand8(0x0B));
  Serial.printf("LCD: RDDID=%02X %02X %02X %02X, COLMOD=%02X\n",
                screen.readcommand8(0x04, 0), screen.readcommand8(0x04, 1),
                screen.readcommand8(0x04, 2), screen.readcommand8(0x04, 3),
                screen.readcommand8(0x0C));
  Serial.printf("LCD: bufor czesciowy RGB565: %s\n", displayUpdates.begin() ? "OK (10 kB)" : "brak pamieci");
  pinMode(21, OUTPUT);
  digitalWrite(21, HIGH);
  touchSPI.begin(25, 39, 32, 33);
  touch.begin(touchSPI);
  touch.setRotation(PanelUI::ROTATION);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  if (configured && String(Config::HA_URL).startsWith("http://")) {
    WiFi.begin(Config::WIFI_SSID, Config::WIFI_PASSWORD);
    lastWifiTry = millis();
  } else if (configured) notice = "HA_URL musi zaczynac sie od http://";
  draw();
}

void loop() {
  const uint32_t now = millis();
  const bool connected = WiFi.status() == WL_CONNECTED;
  if (connected != wasConnected) {
    wasConnected = connected;
    for (auto &s : states) s.known = false;
    boilerHeating.known = boilerTarget.known = false;
    settings.heater.known = settings.climate.known = settings.boiler.known = false;
    if (connected) { polling = true; pollIndex = 0; }
    draw();
  }
  if (configured && !connected && now - lastWifiTry >= 15000 &&
      String(Config::HA_URL).startsWith("http://")) {
    lastWifiTry = now;
    WiFi.reconnect();
  }
  if (notice.length() && noticeUntil && static_cast<int32_t>(now - noticeUntil) >= 0) {
    notice = "";
    noticeUntil = 0;
    draw();
  }
  const bool down = touch.touched();
  if (down) releaseSince = 0;
  if (down && !pressed) {
    TS_Point point = touch.getPoint();
    if (point.z >= 400) {
      pressed = true;
      if (Config::TOUCH_DEBUG) Serial.printf("Touch x=%d y=%d z=%d\n", point.x, point.y, point.z);
      const int x = constrain(map(point.x, Config::TOUCH_X_MIN, Config::TOUCH_X_MAX, 0, PanelUI::WIDTH - 1), 0, PanelUI::WIDTH - 1);
      const int y = constrain(map(point.y, Config::TOUCH_Y_MIN, Config::TOUCH_Y_MAX, 0, PanelUI::HEIGHT - 1), 0, PanelUI::HEIGHT - 1);
      handleTap(x, y);
    }
  }
  if (!down && pressed) {
    if (!releaseSince) releaseSince = millis();
    if (millis() - releaseSince >= 80) pressed = false;
  }
  if (configured && connected) {
    if (!polling && now - lastPoll >= POLL_MS) { polling = true; pollIndex = 0; }
    // Odczyt HA nie zalezy od puszczenia dotyku (takze przy zakloceniach IRQ).
    if (polling) {
      if (pollIndex < 3) readState(pollIndex);
      else if (pollIndex == 3) readSensor(Config::BOILER_HEATING_ENTITY, boilerHeating, true);
      else if (pollIndex == 4) readSensor(Config::BOILER_TARGET_ENTITY, boilerTarget);
      else readBoilerSettings();
      if (++pollIndex == 6) { polling = false; lastPoll = millis(); }
    }
  }
  // Kontroluj aktualnosc bez budowania sygnatury wszystkich opcji co 15 ms.
  static uint32_t lastViewCheck = 0;
  if (millis() - lastViewCheck >= 250) {
    lastViewCheck = millis();
    draw();
  }
  delay(15);
}
