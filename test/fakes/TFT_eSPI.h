#pragma once
#include <algorithm>
#include <stdint.h>
#include <vector>

// Model pamieci LCD do testu skladania i przesylania fragmentow obrazu.
class TFT_eSPI {
 public:
  virtual ~TFT_eSPI() = default;
  int width = 320, height = 240, originX = 0, originY = 0;
  unsigned pixelsWritten = 0;
  std::vector<uint16_t> pixels = std::vector<uint16_t>(320 * 240);
  virtual void fillRect(int x, int y, int w, int h, uint16_t color) {
    x += originX; y += originY;
    for (int py = std::max(0, y); py < std::min(height, y + h); ++py)
      for (int px = std::max(0, x); px < std::min(width, x + w); ++px)
        pixels[py * width + px] = color;
  }
  void startWrite() {}
  void endWrite() {}
};
class TFT_eSprite : public TFT_eSPI {
 public:
  explicit TFT_eSprite(TFT_eSPI *target) : screen(target) {}
  void setColorDepth(int depth) { colorDepth = depth; }
  void *createSprite(int w, int h) {
    width = w; height = h;
    pixels.assign(w * h, 0);
    return pixels.data();
  }
  void resetViewport() { originX = originY = 0; }
  void setOrigin(int x, int y) { originX = x; originY = y; }
  void *getPointer() { return pixels.data(); }
  bool pushSprite(int tx, int ty, int sx, int sy, int w, int h) {
    for (int y = 0; y < h; ++y)
      for (int x = 0; x < w; ++x)
        screen->pixels[(ty + y) * screen->width + tx + x] = pixels[(sy + y) * width + sx + x];
    screen->pixelsWritten += w * h;
    return true;
  }
  int colorDepth = 0;
 private:
  TFT_eSPI *screen;
};
