#ifndef AY_CHANNEL_ROUTING_H
#define AY_CHANNEL_ROUTING_H
#include <stdint.h>
#include <avr/pgmspace.h>

// Labels give logical sources on physical outputs A, B, C.
static const uint8_t channelDestinations[6][3] PROGMEM = {
  {0,1,2}, {0,2,1}, {1,0,2}, {2,0,1}, {1,2,0}, {2,1,0}
};
static const char channelLabels[6][4] PROGMEM = {"ABC","ACB","BAC","BCA","CAB","CBA"};
inline uint8_t channelDestination(uint8_t mode, uint8_t source) {
  return pgm_read_byte(&channelDestinations[mode % 6][source]);
}
inline uint8_t routedRegister(uint8_t mode, uint8_t reg) {
  if (reg < 6) return channelDestination(mode, reg / 2) * 2 + (reg & 1);
  if (reg >= 8 && reg <= 10) return channelDestination(mode, reg - 8) + 8;
  return reg;
}
inline uint8_t routedMixer(uint8_t mode, uint8_t value) {
  uint8_t result = value & 0xC0; // preserve I/O port direction
  for (uint8_t source = 0; source < 3; ++source) {
    uint8_t dest = channelDestination(mode, source);
    if (value & (1 << source)) result |= 1 << dest;
    if (value & (1 << (source + 3))) result |= 1 << (dest + 3);
  }
  return result;
}
#endif
