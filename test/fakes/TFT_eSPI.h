#pragma once
#include <algorithm>
#include <stdint.h>
#include <vector>

// Model pamieci LCD do testu skladania i przesylania fragmentow obrazu.
class TFT_eSPI {
 public:
  virtual ~TFT_eSPI() = default;
  int pixelWidth = 320, pixelHeight = 240, originX = 0, originY = 0;
  virtual int width() const { return pixelWidth; }
  virtual int height() const { return pixelHeight; }
  int getOriginY() const { return originY; }
  unsigned pixelsWritten = 0;
  std::vector<uint16_t> pixels = std::vector<uint16_t>(320 * 240);
  virtual void fillRect(int x, int y, int w, int h, uint16_t color) {
    x += originX; y += originY;
    for (int py = std::max(0, y); py < std::min(pixelHeight, y + h); ++py)
      for (int px = std::max(0, x); px < std::min(pixelWidth, x + w); ++px)
        pixels[py * pixelWidth + px] = color;
  }
  void drawPixel(int x, int y, uint16_t color) { fillRect(x, y, 1, 1, color); }
  void drawFastHLine(int x, int y, int w, uint16_t color) { fillRect(x, y, w, 1, color); }
  void drawFastVLine(int x, int y, int h, uint16_t color) { fillRect(x, y, 1, h, color); }
  void drawLine(int x0, int y0, int x1, int y1, uint16_t color) {
    const int dx = std::abs(x1-x0), dy = -std::abs(y1-y0);
    const int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
    int error = dx + dy;
    for (;;) {
      drawPixel(x0,y0,color);
      if (x0 == x1 && y0 == y1) break;
      const int twice = 2 * error;
      if (twice >= dy) { error += dy; x0 += sx; }
      if (twice <= dx) { error += dx; y0 += sy; }
    }
  }
  void fillCircle(int x, int y, int r, uint16_t color) {
    for (int py=-r; py<=r; ++py) for(int px=-r; px<=r; ++px)
      if(px*px+py*py<=r*r) drawPixel(x+px,y+py,color);
  }
  void drawCircle(int x, int y, int r, uint16_t color) {
    int px=r, py=0, err=1-r;
    while(px>=py) {
      for(int sign : {-1,1}) for(int sign2 : {-1,1}) {
        drawPixel(x+sign*px,y+sign2*py,color);
        drawPixel(x+sign*py,y+sign2*px,color);
      }
      ++py;
      if(err<0) err+=2*py+1; else { --px; err+=2*(py-px)+1; }
    }
  }
  static bool rounded(int x, int y, int w, int h, int r) {
    if(x<0 || y<0 || x>=w || y>=h) return false;
    const int cx=x<r?r-1:(x>=w-r?w-r:x), cy=y<r?r-1:(y>=h-r?h-r:y);
    return (x-cx)*(x-cx)+(y-cy)*(y-cy)<=r*r;
  }
  void fillRoundRect(int x, int y, int w, int h, int r, uint16_t color) {
    for(int py=std::max(0,-y-originY); py<std::min(h,pixelHeight-y-originY); ++py)
      for(int px=0; px<w; ++px) if(rounded(px,py,w,h,r)) drawPixel(x+px,y+py,color);
  }
  void drawRoundRect(int x, int y, int w, int h, int r, uint16_t color) {
    for(int py=std::max(0,-y-originY); py<std::min(h,pixelHeight-y-originY); ++py)
      for(int px=0; px<w; ++px)
        if(rounded(px,py,w,h,r) && !rounded(px-1,py-1,w-2,h-2,std::max(0,r-1))) drawPixel(x+px,y+py,color);
  }
  void fillTriangle(int x0,int y0,int x1,int y1,int x2,int y2,uint16_t color) {
    const auto edge=[](int ax,int ay,int bx,int by,int x,int y){ return (x-ax)*(by-ay)-(y-ay)*(bx-ax); };
    for(int y=std::min({y0,y1,y2}); y<=std::max({y0,y1,y2}); ++y)
      for(int x=std::min({x0,x1,x2}); x<=std::max({x0,x1,x2}); ++x) {
        const int a=edge(x0,y0,x1,y1,x,y), b=edge(x1,y1,x2,y2,x,y), d=edge(x2,y2,x0,y0,x,y);
        if((a>=0&&b>=0&&d>=0)||(a<=0&&b<=0&&d<=0)) drawPixel(x,y,color);
      }
  }
  void startWrite() {}
  void endWrite() {}
};
class TFT_eSprite : public TFT_eSPI {
 public:
  explicit TFT_eSprite(TFT_eSPI *target) : screen(target) {}
  void setColorDepth(int depth) { colorDepth = depth; }
  void *createSprite(int w, int h) {
    pixelWidth = w; pixelHeight = h;
    pixels.assign(w * h, 0);
    return pixels.data();
  }
  void resetViewport() { originX = originY = 0; }
  void setOrigin(int x, int y) { originX = x; originY = y; }
  void *getPointer() { return pixels.data(); }
  bool pushSprite(int tx, int ty, int sx, int sy, int w, int h) {
    for (int y = 0; y < h; ++y)
      for (int x = 0; x < w; ++x)
        screen->pixels[(ty + y) * screen->pixelWidth + tx + x] = pixels[(sy + y) * pixelWidth + sx + x];
    screen->pixelsWritten += w * h;
    return true;
  }
  int colorDepth = 0;
 private:
  TFT_eSPI *screen;
};
