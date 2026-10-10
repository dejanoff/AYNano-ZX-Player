#ifndef AY_PLAYER_DISPLAY_H
#define AY_PLAYER_DISPLAY_H
#include <stdint.h>
#include <stdio.h>
#include <string.h>

inline void folderDisplayLine(char* line, uint16_t index, uint16_t count, const char* path) {
  snprintf(line, 22, "%u/%u ", index + 1, count);
  uint8_t used = strlen(line);
  // AVR printf does not support '*' width/precision. Append explicitly.
  strncat(line, strcmp(path, "/") == 0 ? "ROOT" : path, 21 - used);
}

inline void trackDisplayLine(char* line, uint16_t index, uint16_t count, uint8_t percent) {
  snprintf(line, 22, "%u/%u %u%%", count ? index + 1 : 0, count, percent);
}

inline uint8_t insetBarWidth(uint32_t position, uint32_t size) {
  if (!size) return 0;
  if (position >= size) return 122;
  return uint8_t(uint64_t(position) * 122 / size);
}

inline uint8_t insetBarColumn(uint8_t x, uint8_t width) {
  // Frame at y=1/6. Thin fill at y=3/4, with one clear pixel on every side.
  if (x == 0 || x == 127) return 0x7E;
  return x >= 2 && x < uint16_t(width) + 2 ? 0x5A : 0x42;
}

inline uint8_t textPageColumn(uint8_t glyph, uint8_t row) {
  return (glyph & 0x7F) << row;
}

inline uint8_t footerDisplayColumn(uint8_t x, uint8_t width, uint8_t textPixel, bool tall = false, uint8_t phase = 0) {
  // Six-pixel capsule at y=26..31. Pixel y=25 separates it from the text.
  uint8_t outline = tall ? (x == 0 || x == 127 ? 0x3C : x == 1 || x == 126 ? 0x42 : 0x81) :
    (x == 0 || x == 127 ? 0x30 : x == 1 || x == 126 ? 0x48 : 0x84);
  uint8_t hatch = 0;
  if (x >= 3 && x < uint16_t(width) + 3) {
    // Moving diagonal dashes, clipped to the real progress endpoint.
    for (uint8_t y = tall ? 2 : 4; y <= 5; ++y)
      if (((x + y + 6 - phase % 6) % 6) < 3) hatch |= uint8_t(1 << y);
  }
  return outline | hatch | (tall ? 0 : textPixel & 1);
}

inline bool transportIconPixel(uint8_t state, uint8_t x, uint8_t y, uint8_t emphasis = 0) {
  if (x >= 24 || y >= 24) return false;
  if (emphasis > 2) emphasis = 2;
  // Standalone transport symbols, with room for a brief two-pixel pulse.
  if (state == 0 && y >= 4 - emphasis && y <= 19 + emphasis) {
    uint8_t reach = (y <= 11 ? y - (4 - emphasis) : 19 + emphasis - y) *
      (12 + 2 * emphasis) / (7 + emphasis);
    return x >= 6 - emphasis && x <= 6 - emphasis + reach;
  }
  if (state == 1) return y >= 5 - emphasis && y <= 18 + emphasis &&
    ((x >= 6 - emphasis && x <= 9 + emphasis / 2) ||
     (x >= 14 - emphasis / 2 && x <= 17 + emphasis));
  return state == 2 && x >= 6 - emphasis && x <= 17 + emphasis &&
    y >= 6 - emphasis && y <= 17 + emphasis;
}

inline uint8_t transportIconColumn(uint8_t state, uint8_t x, uint8_t page, uint8_t emphasis = 0) {
  uint8_t result = 0;
  for (uint8_t bit = 0; bit < 8; ++bit)
    if (transportIconPixel(state, x, page * 8 + bit, emphasis)) result |= uint8_t(1 << bit);
  return result;
}

inline uint8_t startupWaveColumn(uint8_t x, uint8_t phase) {
  if (x % 4 == 3) return 0;
  uint8_t step = (x / 4 + phase) % 8;
  uint8_t height = 2 + (step < 4 ? step : 7 - step);
  return uint8_t(((1 << height) - 1) << ((8 - height) / 2));
}
#endif
