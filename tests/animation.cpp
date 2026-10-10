#include "../firmware/AYPlayerNano/PlayerAnimation.h"
volatile uint8_t test_done = 0, test_failure = 0, test_count = 0;
void check(bool ok) {
  ++test_count;
  if (!ok && !test_failure) test_failure = test_count;
}
int main() {
  PlayerAnimation animation;
  check(sizeof(animation) == 5);
  check(animation.emphasis(50) == 0);
  check(animation.nameColumns(50) == 13);
  animation.trigger(100);
  check(animation.emphasis(100) == 0);
  check(animation.emphasis(180) == 1);
  check(animation.emphasis(260) == 2);
  check(animation.emphasis(340) == 1);
  check(animation.emphasis(420) == 0);
  check(animation.nameColumns(180) == 13);
  animation.expire(579);
  check(animation.kind == 1);
  animation.expire(580);
  check(animation.kind == 0);
  animation.trigger(1000, true);
  check(animation.nameColumns(1000) == 3);
  check(animation.nameColumns(1080) == 5);
  check(animation.nameColumns(1320) == 11);
  check(animation.nameColumns(1400) == 13);
  animation.trigger(0xFFFFFFD0UL, true);
  check(animation.emphasis(0x20) == 1);
  check(animation.nameColumns(0x20) == 5);
  animation.expire(0x1B0);
  check(animation.kind == 0);
  animation.trigger(2000, true);
  animation.trigger(2050); // buttons interrupt the reveal, showing full name
  check(animation.nameColumns(2050) == 13);
  test_done = 0xA5;
  for (;;) {}
}
