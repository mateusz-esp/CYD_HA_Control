#include "dirty_tiles.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main() {
  DirtyTiles tiles;
  uint16_t pixels[DirtyTiles::WIDTH * DirtyTiles::TILE] = {};
  const uint32_t full = (1U << DirtyTiles::COLUMNS) - 1;
  // Pierwsze rysowanie wypelnia LCD. Identyczny odczyt HA nie wysyla pikseli.
  for (int row = 0; row < DirtyTiles::ROWS; ++row) {
    assert(tiles.updateRow(row, pixels) == full);
    assert(tiles.updateRow(row, pixels) == 0);
  }
  // Zmiana cyfry/przycisku dotyka tylko odpowiednich kolumn, nie calego ekranu.
  pixels[3 * DirtyTiles::WIDTH + 110] = 0xFFFF;
  assert(tiles.updateRow(4, pixels) == (1U << 6));
  assert(tiles.updateRow(4, pixels) == 0);
  pixels[3 * DirtyTiles::WIDTH + 112] = 0xF800;
  assert(tiles.updateRow(4, pixels) == (1U << 7));
  // Usuniecie starej liczby musi tez odswiezyc jej poprzednie piksele.
  memset(pixels, 0, sizeof(pixels));
  assert(tiles.updateRow(4, pixels) == ((1U << 6) | (1U << 7)));
  assert(tiles.updateRow(4, pixels) == 0);
  // Ostatni piksel i ostatni pas ekranu nie moga wypasc z obszaru aktualizacji.
  pixels[sizeof(pixels) / sizeof(pixels[0]) - 1] = 0x07E0;
  assert(tiles.updateRow(DirtyTiles::ROWS - 1, pixels) == (1U << 19));
  tiles.invalidate();
  assert(tiles.updateRow(0, pixels) == full);
  puts("OK: unchanged frames, local updates, erased text, screen edges, invalidation");
}
