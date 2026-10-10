#include "../firmware/AYPlayerNano/ChannelRouting.h"
volatile uint8_t test_done = 0, test_failure = 0, test_count = 0;
void check(bool ok) { ++test_count; if (!ok && !test_failure) test_failure = test_count; }
int main() {
  for (uint8_t mode = 0; mode < 6; ++mode) {
    bool valid = true;
    uint8_t seen = 0;
    for (uint8_t source = 0; source < 3; ++source) {
      uint8_t dest = channelDestination(mode, source);
      seen |= 1 << dest;
      valid &= pgm_read_byte(&channelLabels[mode][dest]) == 'A' + source;
      valid &= routedRegister(mode, source * 2) == dest * 2;
      valid &= routedRegister(mode, source * 2 + 1) == dest * 2 + 1;
      valid &= routedRegister(mode, 8 + source) == 8 + dest;
    }
    check(valid && seen == 7);
    bool mixerOk = true;
    for (uint16_t value = 0; value < 256; ++value) {
      uint8_t mapped = routedMixer(mode, value);
      mixerOk &= (mapped & 0xC0) == (value & 0xC0);
      for (uint8_t source = 0; source < 3; ++source) {
        uint8_t dest = channelDestination(mode, source);
        mixerOk &= ((value >> source) & 1) == ((mapped >> dest) & 1);
        mixerOk &= ((value >> (source + 3)) & 1) == ((mapped >> (dest + 3)) & 1);
      }
    }
    check(mixerOk);
    check(routedRegister(mode, 6) == 6 && routedRegister(mode, 7) == 7);
    check(routedRegister(mode, 11) == 11 && routedRegister(mode, 12) == 12 && routedRegister(mode, 13) == 13);
  }
  check(routedMixer(1, 0xD2) == 0xE4);
  test_done = 0xA5;
  for (;;) {}
}
