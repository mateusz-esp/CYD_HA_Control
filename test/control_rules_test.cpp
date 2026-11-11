#include "control_rules.h"
#include <assert.h>
#include <stdio.h>

int main() {
  using namespace ControlRules;
  // Rzeczywiste zakresy odczytane z HA: Ariston 40..80, klimatyzacja 8..30.
  assert(adjust(40, -1, 40, 80, 1) == 40);
  assert(adjust(80, 1, 40, 80, 1) == 80);
  assert(adjust(55, 1, 40, 80, 1) == 56);
  assert(canSave(55, 40, 80, 1));
  assert(!canSave(39, 40, 80, 1));
  assert(!canSave(81, 40, 80, 1));
  assert(!canSave(55.5, 40, 80, 1));
  assert(adjust(21, 1, 8, 30, 0.5f) == 21.5f);
  assert(canSave(21.5f, 8, 30, 0.5f));
  // Grzejnik w kuchni: nie przenos zakresu klimatyzacji ani bojlera.
  assert(adjust(5, -1, 5, 40, 1) == 5);
  assert(adjust(40, 1, 5, 40, 1) == 40);
  assert(canSave(19, 5, 40, 1));
  assert(!canSave(41, 5, 40, 1));
  assert(!canSave(NAN, 40, 80, 1));
  assert(!canSave(INFINITY, 40, 80, 1));
  assert(!canSave(55, NAN, 80, 1));
  assert(!canSave(55, 40, 80, 0));
  assert(!canSave(55, 80, 40, 1));
  assert(isnan(adjust(NAN, 1, 40, 80, 1)));
  puts("OK: temperature bounds, steps, missing and invalid metadata");
}
