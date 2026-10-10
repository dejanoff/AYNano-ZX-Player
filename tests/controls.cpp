#include "../firmware/AYPlayerNano/PlayerControls.h"
#include "../firmware/AYPlayerNano/PlayerBookmark.h"
volatile uint8_t test_done = 0, test_failure = 0, test_count = 0;
void check(bool ok) {
  ++test_count;
  if (!ok && !test_failure) test_failure = test_count;
}
int main() {
  PlayerControls keys;
  check(keys.update(0, 0) == 0);
  check(keys.update(0, 40) == 0);
  check(keys.update(3, 100) == 0);
  check(keys.update(3, 140) == 0);
  check(keys.update(0, 300) == 0);
  check(keys.update(0, 340) == 3); // pause only on release
  keys.update(3, 400); keys.update(3, 440);
  check(keys.update(3, 1399) == 0);
  check(keys.update(3, 1400) == 13);
  check(keys.update(3, 1500) == 0);
  keys.update(0, 1600);
  check(keys.update(0, 1640) == 0); // no pause after long stop
  keys.update(4, 1700); keys.update(4, 1740);
  check(keys.update(4, 3699) == 0);
  check(keys.update(4, 3700) == 14);
  keys.update(0, 3800);
  check(keys.update(0, 3840) == 0); // no random toggle after save
  keys.update(4, 3900); keys.update(4, 3940);
  keys.update(0, 4000);
  check(keys.update(0, 4040) == 4);
  keys.update(2, 4100);
  check(keys.update(2, 4140) == 2);
  check(keys.update(2, 6000) == 0); // no repeat while held
  keys.update(0, 6100); keys.update(0, 6140);
  keys.update(6, 6200);
  check(keys.update(6, 6240) == 0); // folder action waits for release
  check(keys.update(6, 7199) == 0);
  check(keys.update(6, 7200) == 16);
  check(keys.update(6, 7400) == 0); // one routing change per hold
  keys.update(0, 7500);
  check(keys.update(0, 7540) == 0); // no folder change after routing
  keys.update(6, 7600); keys.update(6, 7640); keys.update(0, 7800);
  check(keys.update(0, 7840) == 6);
  for (uint16_t current = 0; current < 5; ++current)
    for (uint16_t pick = 0; pick < 4; ++pick) {
      uint16_t result = randomOtherTrack(current, 5, pick);
      check(result < 5 && result != current);
    }
  check(randomOtherTrack(0, 1, 0) == 0);
  check(nextPlaybackMode(Sequential) == Shuffle);
  check(nextPlaybackMode(Shuffle) == RepeatOne);
  check(nextPlaybackMode(RepeatOne) == Sequential);
  check(repeatsCurrentTrack(RepeatOne, true));
  check(!repeatsCurrentTrack(RepeatOne, false));
  check(!repeatsCurrentTrack(Shuffle, true));
  check(!repeatsCurrentTrack(Sequential, true));
  PlayerBookmark b = {};
  b.marker = 0xA7; b.version = 1; b.catalog = 1234;
  b.folderCount = 2; b.trackCount = 53; b.trackIndex = 52;
  b.folder[0] = '/'; b.song[0] = 'A';
  b.checksum = bookmarkChecksum(b);
  check(bookmarkValid(b, 1234, 2));
  check(!bookmarkValid(b, 1235, 2)); // changed catalog
  check(!bookmarkValid(b, 1234, 3)); // changed folder count
  b.song[0] = 'B';
  check(!bookmarkValid(b, 1234, 2)); // damaged data
  b.checksum = bookmarkChecksum(b); b.marker = 0;
  check(!bookmarkValid(b, 1234, 2)); // interrupted commit
  test_done = 0xA5;
  for (;;) {}
}
