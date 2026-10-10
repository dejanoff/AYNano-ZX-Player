#ifndef AY_PSG_TIMING_H
#define AY_PSG_TIMING_H
#include <stdint.h>

inline uint16_t psgWaitTicks(uint8_t count) {
  return count ? uint16_t(count) * 4 : 1;
}

struct PsgDuration {
  uint32_t ticks = 0;
  uint8_t pending = 0; // 1 = register value, 2 = extended wait count
  bool ended = false;
  bool valid = true;
  void add(uint8_t value) {
    if (ended || !valid) return;
    if (pending) {
      if (pending == 2) ticks += psgWaitTicks(value);
      pending = 0;
    } else if (value == 0xFF) ++ticks;
    else if (value == 0xFE) pending = 2;
    else if (value == 0xFD) ended = true;
    else if (value <= 15) pending = 1;
    else valid = false;
  }
};

inline uint8_t psgBarWidth(uint32_t elapsed, uint32_t total) {
  if (!total) return 0;
  if (elapsed >= total) return 126;
  return uint8_t(uint64_t(elapsed) * 126 / total);
}
#endif
