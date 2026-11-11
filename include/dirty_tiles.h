#pragma once
#include <stdint.h>

// Podpisy pikseli, bez kopii calej poprzedniej klatki.
class DirtyTiles {
 public:
  static constexpr int WIDTH = 320, HEIGHT = 240, TILE = 16;
  static constexpr int COLUMNS = WIDTH / TILE, ROWS = HEIGHT / TILE;
  void invalidate() { validRows = 0; }
  uint32_t updateRow(int row, const uint16_t *pixels) {
    uint32_t changed = 0;
    for (int column = 0; column < COLUMNS; ++column) {
      uint32_t hash = 2166136261U;
      for (int y = 0; y < TILE; ++y) {
        for (int x = 0; x < TILE; ++x) {
          const uint16_t pixel = pixels[y * WIDTH + column * TILE + x];
          hash = (hash ^ (pixel & 0xFF)) * 16777619U;
          hash = (hash ^ (pixel >> 8)) * 16777619U;
        }
      }
      const int index = row * COLUMNS + column;
      if (!(validRows & (1U << row)) || hashes[index] != hash) changed |= 1U << column;
      hashes[index] = hash;
    }
    validRows |= 1U << row;
    return changed;
  }
 private:
  uint32_t hashes[ROWS * COLUMNS] = {};
  uint32_t validRows = 0;
};
