#ifndef AY_PLAYER_CONTROLS_H
#define AY_PLAYER_CONTROLS_H
#include <stdint.h>

enum PlaybackMode : uint8_t { Sequential, Shuffle, RepeatOne };
inline PlaybackMode nextPlaybackMode(PlaybackMode mode) {
  return PlaybackMode((uint8_t(mode) + 1) % 3);
}
inline bool repeatsCurrentTrack(PlaybackMode mode, bool automatic) {
  return mode == RepeatOne && automatic;
}

class PlayerControls {
 public:
  // IDs 1..6 are short actions; 13 = stop, 14 = bookmark, 16 = channel routing.
  uint8_t update(uint8_t reading, uint32_t now) {
    if (reading == 255) {
      candidate_ = 255;
      changedAt_ = now;
      return 0;
    }
    if (reading != candidate_) {
      candidate_ = reading;
      changedAt_ = now;
      return 0;
    }
    if (uint32_t(now - changedAt_) < 40) return 0;
    if (reading != stable_) {
      stable_ = reading;
      if (!reading) {
        uint8_t action = !longSent_ && (active_ == 3 || active_ == 4 || active_ == 6) ? active_ : 0;
        uint16_t threshold = active_ == 3 || active_ == 6 ? 1000 : active_ == 4 ? 2000 : 0;
        if (action && threshold && uint32_t(changedAt_ - pressedAt_) >= threshold)
          action += 10;
        active_ = 0;
        armed_ = true;
        return action;
      }
      if (!armed_ || active_) return 0;
      armed_ = false;
      active_ = reading;
      pressedAt_ = changedAt_;
      longSent_ = false;
      if (reading != 3 && reading != 4 && reading != 6) return reading;
    }
    if (active_ && reading == active_ && !longSent_) {
      uint16_t threshold = active_ == 3 || active_ == 6 ? 1000 : active_ == 4 ? 2000 : 0;
      if (threshold && uint32_t(now - pressedAt_) >= threshold) {
        longSent_ = true;
        return active_ + 10;
      }
    }
    return 0;
  }

 private:
  uint8_t candidate_ = 255, stable_ = 255, active_ = 0;
  uint32_t changedAt_ = 0, pressedAt_ = 0;
  bool armed_ = false, longSent_ = false;
};

inline uint16_t randomOtherTrack(uint16_t current, uint16_t count, uint16_t pick) {
  if (count < 2) return 0;
  // pick is uniformly chosen from [0, count-2], excluding current.
  return pick >= current ? pick + 1 : pick;
}
#endif
