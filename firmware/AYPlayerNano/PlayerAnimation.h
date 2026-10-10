#ifndef AY_PLAYER_ANIMATION_H
#define AY_PLAYER_ANIMATION_H
#include <stdint.h>

// Five bytes of state; no framebuffer, delays or dynamically allocated memory.
struct PlayerAnimation {
  uint32_t started = 0;
  uint8_t kind = 0; // 0 idle, 1 button feedback, 2 new track

  void trigger(uint32_t now, bool newTrack = false) {
    started = now;
    kind = newTrack ? 2 : 1;
  }

  void expire(uint32_t now) {
    if (kind && uint32_t(now - started) >= 480) kind = 0;
  }

  uint8_t emphasis(uint32_t now) const {
    uint32_t elapsed = uint32_t(now - started);
    if (!kind || elapsed >= 320) return 0;
    uint8_t frame = elapsed / 80;
    return frame == 2 ? 2 : frame & 1;
  }

  uint8_t nameColumns(uint32_t now) const {
    uint32_t elapsed = uint32_t(now - started);
    if (kind != 2 || elapsed >= 400) return 13;
    return 3 + 2 * (elapsed / 80);
  }
};
#endif
