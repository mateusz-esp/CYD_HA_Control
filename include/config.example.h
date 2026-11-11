#pragma once

// Skopiuj jako include/config.h. Nie publikuj hasla ani tokena.
namespace Config {
constexpr char WIFI_SSID[] = "WIFI_SSID";
constexpr char WIFI_PASSWORD[] = "WIFI_PASSWORD";
constexpr char HA_URL[] = "HA_URL";
constexpr char HA_TOKEN[] = "HA_TOKEN";
// Zakresy surowe po touch.setRotation(1); mozna odwrocic min/max.
constexpr int TOUCH_X_MIN = 200, TOUCH_X_MAX = 3800;
constexpr int TOUCH_Y_MIN = 200, TOUCH_Y_MAX = 3800;
constexpr bool TOUCH_DEBUG = false;
struct Device {
  const char *label, *entity, *domain, *onService, *offService, *onData, *offData;
};
// Trzy urzadzenia; dodatkowe czujniki bojlera sa tylko do odczytu.
constexpr Device DEVICES[] = {
    {"Grzejnik", "climate.grzejnik", "climate", "turn_on", "turn_off", "{}", "{}"},
    {"Klimatyzacja", "climate.klimatyzacja", "climate", "turn_on", "turn_off", "{}", "{}"},
    {"Bojler", "switch.bojler_zasilanie", "switch", "turn_on", "turn_off", "{}", "{}"},
};
constexpr char BOILER_CONTROL_ENTITY[] = "water_heater.bojler";
constexpr char BOILER_HEATING_ENTITY[] = "binary_sensor.bojler_grzanie";
constexpr char BOILER_TARGET_ENTITY[] = "sensor.bojler_temperatura_zadana";

// Przyklad przelacznika: entity = "switch.grzejnik".
// Przyklad climate (tryb cool musi byc wspierany przez urzadzenie):
// {"Klimatyzacja", "climate.klimatyzacja", "climate", "set_hvac_mode",
//  "set_hvac_mode", "{\"hvac_mode\":\"cool\"}", "{\"hvac_mode\":\"off\"}"},
// Dla water_heater mozna ustawic set_operation_mode i JSON operation_mode
// zgodny z operation_list konkretnej encji.
}
