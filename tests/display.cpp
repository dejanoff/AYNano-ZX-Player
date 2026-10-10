#include "../firmware/AYPlayerNano/PlayerDisplay.h"
volatile uint8_t test_done = 0, test_failure = 0, test_count = 0;
void check(bool ok) {
  ++test_count;
  if (!ok && !test_failure) test_failure = test_count;
}
int main() {
  char line[22];
  folderDisplayLine(line, 0, 5, "/");
  check(strcmp(line, "1/5 ROOT") == 0);
  folderDisplayLine(line, 1, 5, "/MUSIC");
  check(strcmp(line, "2/5 /MUSIC") == 0);
  folderDisplayLine(line, 65534, 65535, "/LONGPATH12345");
  check(strlen(line) == 21);
  check(strncmp(line, "65535/65535 ", 12) == 0);
  trackDisplayLine(line, 12, 50, 75);
  check(strcmp(line, "13/50 75%") == 0);
  trackDisplayLine(line, 0, 0, 0);
  check(strcmp(line, "0/0 0%") == 0);
  check(insetBarWidth(0, 100) == 0);
  check(insetBarWidth(50, 100) == 61);
  check(insetBarWidth(100, 100) == 122);
  check(insetBarWidth(200, 100) == 122);
  check(insetBarWidth(1, 0) == 0);
  check(insetBarColumn(0, 124) == 0x7E);
  check(insetBarColumn(1, 124) == 0x42);
  check(insetBarColumn(2, 124) == 0x5A);
  check(insetBarColumn(125, 124) == 0x5A);
  check(insetBarColumn(126, 124) == 0x42);
  check(insetBarColumn(127, 124) == 0x7E);
  check(insetBarColumn(2, 0) == 0x42);
  check(insetBarColumn(63, 62) == 0x5A);
  check(insetBarColumn(64, 62) == 0x42);
  check(textPageColumn(0x7F, 0) == 0x7F);
  check(textPageColumn(0x7F, 1) == 0xFE);
  check(textPageColumn(0x7F, 2) == 0xFC);
  check(footerDisplayColumn(0, 122, 1) == 0x31);
  check(footerDisplayColumn(1, 122, 1) == 0x49);
  check(footerDisplayColumn(2, 122, 1) == 0x85);
  check(footerDisplayColumn(124, 122, 1) == 0x95);
  check(footerDisplayColumn(125, 122, 0) == 0x84);
  check((footerDisplayColumn(2, 122, 0) & 128) != 0);
  check(footerDisplayColumn(0, 122, 1, true) == 0x3C);
  check(footerDisplayColumn(1, 122, 1, true) == 0x42);
  check(footerDisplayColumn(2, 122, 1, true) == 0x81);
  check(footerDisplayColumn(3, 0, 0, true) == 0x81);
  check((footerDisplayColumn(5, 122, 0, true) & 0x7E) !=
    (footerDisplayColumn(8, 122, 0, true) & 0x7E));
  for (uint8_t x = 0; x < 128; ++x)
    check((footerDisplayColumn(x, 122, 1) & 2) == 0); // clear y=25 everywhere
  check(transportIconPixel(0, 6, 4));
  check(transportIconPixel(0, 18, 11));
  check(!transportIconPixel(0, 18, 4));
  check(transportIconPixel(1, 6, 8));
  check(transportIconPixel(1, 16, 8));
  check(!transportIconPixel(1, 12, 8));
  check(transportIconPixel(2, 6, 6));
  check(transportIconPixel(2, 17, 17));
  check(!transportIconPixel(2, 5, 6));
  check(!transportIconPixel(0, 24, 10));
  check(!transportIconPixel(0, 10, 24));
  check(transportIconColumn(2, 0, 0) == 0);
  check(transportIconColumn(2, 6, 1) == 0xFF);
  check(transportIconColumn(1, 12, 1) == 0);
  bool borderClear = true, barSafe = true, waveSafe = true;
  for (uint8_t state = 0; state < 3; ++state)
    for (uint8_t pulse = 0; pulse < 3; ++pulse)
      for (uint8_t edge = 0; edge < 24; ++edge)
        borderClear &= !transportIconPixel(state, edge, 0, pulse) &&
          !transportIconPixel(state, edge, 23, pulse) &&
          !transportIconPixel(state, 0, edge, pulse) &&
          !transportIconPixel(state, 23, edge, pulse);
  check(borderClear);
  check(transportIconPixel(2, 4, 4, 2));
  check(!transportIconPixel(2, 4, 4));
  check(!transportIconPixel(1, 11, 8, 2));
  check(!transportIconPixel(1, 12, 8, 2));
  for (uint8_t phase = 0; phase < 6; ++phase)
    for (uint8_t x = 0; x < 128; ++x) {
      uint8_t column = footerDisplayColumn(x, 61, 1, false, phase);
      barSafe &= (column & 0xCF) == (footerDisplayColumn(x, 0, 1) & 0xCF);
      if (x < 3 || x >= 64) barSafe &= (column & 0x30) ==
        (footerDisplayColumn(x, 0, 1) & 0x30);
    }
  check(barSafe); // animated fill preserves border, gap, text and endpoint
  check(footerDisplayColumn(7, 61, 0, false, 0) !=
    footerDisplayColumn(7, 61, 0, false, 2));
  check(footerDisplayColumn(7, 61, 0, false, 0) ==
    footerDisplayColumn(7, 61, 0, false, 6));
  for (uint8_t phase = 0; phase < 8; ++phase)
    for (uint8_t x = 0; x < 48; ++x) {
      uint8_t column = startupWaveColumn(x, phase);
      waveSafe &= (column & 0x81) == 0;
      if (x % 4 == 3) waveSafe &= column == 0;
    }
  check(waveSafe);
  check(startupWaveColumn(0, 0) != startupWaveColumn(0, 3));
  test_done = 0xA5;
  for (;;) {}
}
