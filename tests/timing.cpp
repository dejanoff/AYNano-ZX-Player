#include "../firmware/AYPlayerNano/PsgTiming.h"
volatile uint8_t test_done = 0, test_failure = 0, test_count = 0;
void check(bool ok) {
  ++test_count;
  if (!ok && !test_failure) test_failure = test_count;
}
int main() {
  PsgDuration p;
  const uint8_t data[] = {0, 0xFF, 1, 0xFD, 0xFF, 0xFE, 3, 0xFF, 0xFD, 0xFF};
  for (uint8_t i = 0; i < sizeof(data); ++i) p.add(data[i]);
  check(p.valid && p.ended && !p.pending);
  check(p.ticks == 14); // register values and trailing bytes must not add time
  PsgDuration truncated;
  truncated.add(0xFE);
  check(truncated.pending == 2);
  PsgDuration bad;
  bad.add(0x20);
  check(!bad.valid);
  check(psgWaitTicks(255) == 1020);
  check(psgWaitTicks(0) == 1);
  check(psgBarWidth(0, 100) == 0);
  check(psgBarWidth(50, 100) == 63);
  check(psgBarWidth(100, 100) == 126);
  check(psgBarWidth(101, 100) == 126);
  check(psgBarWidth(100, 0) == 0);
  check(psgBarWidth(2000000000UL, 4000000000UL) == 63);
  test_done = 0xA5;
  for (;;) {}
}
