#pragma once
#include <TFT_eSPI.h>
#include "dirty_tiles.h"

class PartialDisplay {
 public:
  explicit PartialDisplay(TFT_eSPI &display) : screen(display), strip(&display) {}
  bool begin() {
    strip.setColorDepth(16);
    available = strip.createSprite(DirtyTiles::WIDTH, DirtyTiles::TILE) != nullptr;
    return available;
  }
  template <typename DrawScene> void present(DrawScene drawScene) {
    if (!available) {
      // Tryb awaryjny przy braku 10 kB ciaglej pamieci.
      drawScene(screen);
      return;
    }
    for (int row = 0; row < DirtyTiles::ROWS; ++row) {
      strip.resetViewport();
      strip.setOrigin(0, -row * DirtyTiles::TILE);
      drawScene(strip);
      // pushSprite operuje na lokalnych wspolrzednych bufora.
      strip.setOrigin(0, 0);
      const uint32_t changed = tiles.updateRow(row, static_cast<const uint16_t *>(strip.getPointer()));
      for (int column = 0; column < DirtyTiles::COLUMNS;) {
        if (!(changed & (1U << column))) { ++column; continue; }
        const int first = column++;
        while (column < DirtyTiles::COLUMNS && (changed & (1U << column))) ++column;
        screen.startWrite();
        strip.pushSprite(first * DirtyTiles::TILE, row * DirtyTiles::TILE,
                         first * DirtyTiles::TILE, 0,
                         (column - first) * DirtyTiles::TILE, DirtyTiles::TILE);
        screen.endWrite();
      }
    }
  }
 private:
  TFT_eSPI &screen;
  TFT_eSprite strip;
  DirtyTiles tiles;
  bool available = false;
};
