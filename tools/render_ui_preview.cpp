// Uses the firmware's actual views/fonts with a desktop LCD model.
#include "ui_views.h"
#include "partial_display.h"
#include <assert.h>
#include <fstream>
#include <stdio.h>

void save(const char *name, TFT_eSPI &screen) {
  std::ofstream out(name, std::ios::binary);
  out << "P6\n320 240\n255\n";
  for (auto pixel : screen.pixels) {
    const char rgb[] = {char(((pixel >> 11) & 31) * 255 / 31),
                        char(((pixel >> 5) & 63) * 255 / 63), char((pixel & 31) * 255 / 31)};
    out.write(rgb, 3);
  }
}
int main() {
  TFT_eSPI canvas, reference;
  PartialDisplay updates(canvas);
  assert(updates.begin());
  Views::DeviceTile devices[] = {
    {"Grzejnik", "Wyłączony", "24°", "Cel 19°", false, true},
    {"Klimatyzacja", "Grzanie", "22°", "Cel 21°", true, true},
    {"Bojler", "Nie grzeje", "54°", "Cel 55°", true, true},
  };
  auto home = [&](TFT_eSPI &c) { Views::home(c, devices, true, ""); };
  updates.present(home); home(reference);
  assert(canvas.pixels == reference.pixels);
  save("docs/previews/home.ppm", canvas);
  canvas.pixelsWritten = 0;
  updates.present(home);
  assert(canvas.pixelsWritten == 0);
  Views::Detail detail = {};
  detail.device = 1;
  detail.available = detail.temperatureAvailable = detail.dirty = detail.saveAvailable = true;
  detail.target = "22°"; detail.current = "Teraz: 22°";
  detail.range = "Zakres: 8 - 30°"; detail.step = "Krok regulacji: 1°";
  detail.count = 5;
  detail.fields[0] = {"Tryb pracy", "Grzanie", true};
  detail.fields[1] = {"Wentylator", "Niski", true};
  detail.fields[2] = {"Profil", "Normalny", true};
  detail.fields[3] = {"Nawiew pionowy", "Środek", true};
  detail.fields[4] = {"Nawiew poziomy", "Środek", true};
  auto settings = [&](TFT_eSPI &c) { Views::detail(c, detail, ""); };
  updates.present(settings); settings(reference);
  assert(canvas.pixels == reference.pixels);
  save("docs/previews/climate.ppm", canvas);
  canvas.pixelsWritten = 0;
  detail.target = "23°";
  updates.present(settings); settings(reference);
  assert(canvas.pixels == reference.pixels);
  assert(canvas.pixelsWritten > 0 && canvas.pixelsWritten < 320U * 240U / 4);
  const unsigned changed = canvas.pixelsWritten;
  detail.device = 2; detail.target = "55°"; detail.current = "Woda: 54°";
  detail.range = "Zakres: 40 - 80°"; detail.count = 1;
  detail.fields[0] = {"Tryb bojlera", "Ręczny", true};
  updates.present(settings); settings(reference);
  assert(canvas.pixels == reference.pixels);
  save("docs/previews/boiler.ppm", canvas);
  Views::Option rows[] = {{"Auto",false},{"Niski",true},{"Średnio niski",false},{"Średni",false}};
  auto choices = [&](TFT_eSPI &c) { Views::options(c,1,"Wentylator",rows,4,0,2,true,""); };
  updates.present(choices); choices(reference);
  assert(canvas.pixels == reference.pixels);
  save("docs/previews/options.ppm", canvas);
  printf("OK: actual views + Polish glyphs match full-frame render; temperature edit: %u/76800 pixels\n", changed);
}
