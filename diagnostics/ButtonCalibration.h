#ifndef AY_BUTTON_CALIBRATION_H
#define AY_BUTTON_CALIBRATION_H

#include <Arduino.h>

// Measured on this Nano on 2026-10-10. IDs follow the user's numbering.
// An unmatched reading is ignored, never converted to a nearby button.
inline uint8_t ayButtonId(uint16_t raw) {
  const uint16_t centres[] = {672, 629, 574, 501, 399, 246};
  for (uint8_t i = 0; i < 6; ++i) {
    if (raw >= centres[i] - 15 && raw <= centres[i] + 15) return i + 1;
  }
  return 0;
}

class AyButtonInput {
 public:
  // One event per press, only after a stable release since the last press.
  uint8_t update(uint16_t raw, unsigned long now) {
    uint8_t reading = raw >= 950 ? 0 : ayButtonId(raw);
    // Ignore gaps between valid windows; they do not count as a release.
    if (raw < 950 && reading == 0) {
      candidate_ = 255;
      changedAt_ = now;
      return 0;
    }
    if (reading != candidate_) {
      candidate_ = reading;
      changedAt_ = now;
      return 0;
    }
    if (now - changedAt_ < 40) return 0;
    if (reading == 0) {
      armed_ = true;
      return 0;
    }
    if (!armed_) return 0;
    armed_ = false;
    return reading;
  }

 private:
  uint8_t candidate_ = 255;
  unsigned long changedAt_ = 0;
  bool armed_ = false;
};

#endif
