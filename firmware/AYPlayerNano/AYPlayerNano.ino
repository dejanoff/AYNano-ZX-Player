#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <SSD1306Ascii.h>
#include <SSD1306AsciiWire.h>
#include <util/atomic.h>
#include <EEPROM.h>
#include "ButtonCalibration.h"
#include "PsgTiming.h"
#include "PlayerControls.h"
#include "PlayerBookmark.h"
#include "PlayerDisplay.h"
#include "FirmwareVersion.h"
#include "ChannelRouting.h"

// ATmega328P Nano: original AY wiring, OLED A4/A5, resistor keys A6.
SSD1306AsciiWire oled;
PlayerControls keys;
File music;
char folder[14] = "/";
char song[13] = "";
uint16_t folderIndex = 0, folderCount = 1, trackIndex = 0, trackCount = 0;
bool displayReady = false, sdReady = false, paused = false;
PlaybackMode playbackMode = Sequential;
bool eofQueued = false;
bool playbackPrepared = false;
bool stopped = false;
uint32_t catalogSignature = 2166136261UL;
uint8_t playableFolders[32] = {0}; // eligibility of the first 256 root directories
unsigned long savedAt = 0;
bool saveNotice = false;
const __FlashStringHelper* statusText = nullptr;
uint32_t dataSize = 0;
char screenText[3][17] = {{0}}; // exactly the 17 visible text columns
uint8_t screenIcon = 255, screenMode = 255, screenBar = 255;
unsigned long lastDisplay = 0;
volatile uint8_t head = 0, tail = 0;
volatile uint8_t buffer[128];
volatile bool playing = false, finished = false;
volatile uint16_t waitFrames = 0, underruns = 0;
volatile uint32_t consumed = 0;
volatile uint8_t levels[3] = {0, 0, 0};
volatile uint8_t channelMode = 0;
volatile uint8_t logicalRegisters[14] = {0};
const uint8_t modeSymbols[3][12] PROGMEM = {
  {0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x3F,0x12,0},
  {0x42,0x42,0x24,0x24,0x18,0x18,0x24,0x24,0x42,0xE7,0x42,0},
  {0x3E,0x41,0x49,0x7F,0x41,0x41,0x41,0x41,0x3E,0x1C,0x08,0}
};

void ayHardwareWrite(uint8_t reg, uint8_t value) {
  PORTB &= 0xFC;
  PORTC = (PORTC & 0xF0) | (reg & 0x0F);
  PORTD &= 0x0F;
  PORTB |= 3;
  delayMicroseconds(1);
  PORTB &= 0xFC;
  PORTC = (PORTC & 0xF0) | (value & 0x0F);
  PORTD = (PORTD & 0x0F) | (value & 0xF0);
  PORTB |= 2;
  delayMicroseconds(1);
  PORTB &= 0xFC;
}

void ayWrite(uint8_t reg, uint8_t value) {
  if (reg < 14) logicalRegisters[reg] = value;
  ayHardwareWrite(routedRegister(channelMode, reg), reg == 7 ? routedMixer(channelMode, value) : value);
}

void silence() {
  for (uint8_t i = 0; i < 3; ++i) ayHardwareWrite(8 + i, 0);
}

void cycleChannelMode() {
  bool resume;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { resume = playing; playing = false; }
  silence();
  channelMode = (channelMode + 1) % 6;
  // Reapply cached tone/mixer/volume state without restarting the song or
  // retriggering the shared envelope (R13). Paused/stopped players stay silent.
  for (uint8_t reg = 0; reg < 6; ++reg)
    ayHardwareWrite(routedRegister(channelMode, reg), logicalRegisters[reg]);
  ayHardwareWrite(7, routedMixer(channelMode, logicalRegisters[7]));
  if (resume && !finished)
    for (uint8_t reg = 8; reg <= 10; ++reg)
      ayHardwareWrite(routedRegister(channelMode, reg), logicalRegisters[reg]);
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { playing = resume; }
  Serial.print(F("CHANNELS "));
  for (uint8_t i = 0; i < 3; ++i) Serial.write(pgm_read_byte(&channelLabels[channelMode][i]));
  Serial.println();
}

void stopPlayback() {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { playing = false; }
  silence();
  music.close();
  head = tail = 0;
  waitFrames = 0;
  finished = false;
  consumed = 0;
  dataSize = 0;
  eofQueued = false;
  playbackPrepared = false;
  paused = false;
  stopped = false;
  for (uint8_t i = 0; i < 3; ++i) levels[i] = 0;
}

void fillBuffer() {
  if (!playbackPrepared || !music || eofQueued) return;
  while (uint8_t(head - tail) < 127) {
    int value = music.read();
    buffer[head & 127] = value < 0 ? 0xFD : uint8_t(value);
    ++head; // publish only after the byte is stored; 8-bit access is atomic
    if (value < 0) { eofQueued = true; break; }
  }
}

ISR(TIMER1_COMPA_vect) {
  if (!playing || finished) return;
  if (waitFrames) { --waitFrames; return; }
  uint8_t available = uint8_t(head - tail);
  uint8_t p = tail, remaining = available;
  bool complete = false;
  // Validate an entire frame before writing AY registers. Incomplete frames
  // remain queued; they are never applied repeatedly during an SD underrun.
  while (remaining) {
    uint8_t command = buffer[p++ & 127];
    --remaining;
    if (command == 0xFF || command == 0xFD) { complete = true; break; }
    if (!remaining) break;
    ++p;
    --remaining;
    if (command == 0xFE) { complete = true; break; }
    if (command > 15) { finished = true; silence(); return; }
  }
  if (!complete) {
    if (available == 127) { finished = true; silence(); }
    else if (underruns != 65535) ++underruns;
    return;
  }
  uint8_t end = p;
  p = tail;
  while (p != end) {
    uint8_t command = buffer[p++ & 127];
    if (command == 0xFD) { finished = true; silence(); break; }
    if (command == 0xFF) break;
    uint8_t value = buffer[p++ & 127];
    if (command == 0xFE) {
      waitFrames = psgWaitTicks(value) - 1;
      break;
    }
    ayWrite(command, value);
    if (command >= 8 && command <= 10) levels[command - 8] = value;
  }
  consumed += uint8_t(end - tail);
  tail = end;
}

bool isPsg(File& entry) {
  if (entry.isDirectory()) return false;
  const char* name = entry.name();
  uint8_t n = strlen(name);
  return n >= 4 && name[n-4] == '.' &&
    toupper(name[n-3]) == 'P' && toupper(name[n-2]) == 'S' && toupper(name[n-1]) == 'G';
}

uint16_t countTracks(const char* path) {
  uint16_t count = 0;
  File dir = SD.open(path);
  if (!dir || !dir.isDirectory()) { dir.close(); return 0; }
  dir.rewindDirectory();
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) break;
    if (isPsg(entry) && count < 65535) ++count;
    entry.close();
  }
  dir.close();
  return count;
}

void scanFolders() {
  folderCount = 1; // root always included, even when empty
  File dir = SD.open("/");
  dir.rewindDirectory();
  catalogSignature = 2166136261UL;
  memset(playableFolders, 0, sizeof(playableFolders));
  uint16_t directoryNumber = 0;
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) break;
    bool directory = entry.isDirectory();
    if (directory || isPsg(entry)) hashCatalogEntry(entry);
    if (directory) {
      char path[14];
      snprintf(path, sizeof(path), "/%s", entry.name());
      uint32_t resume = dir.position();
      entry.close();
      dir.close();
      // Keep only two SD handles alive. Nested enumeration with three handles
      // and SD's path parser leaves too little stack space on a 2 KB Nano.
      File childDir = SD.open(path);
      childDir.rewindDirectory();
      bool hasTracks = false;
      // Metadata only, once at boot. Do not scan song contents.
      while (true) {
        if (!childDir) break;
        File child = childDir.openNextFile();
        if (!child) break;
        if (isPsg(child)) { hashCatalogEntry(child); hasTracks = true; }
        child.close();
      }
      childDir.close();
      if (hasTracks && directoryNumber < 256) {
        playableFolders[directoryNumber / 8] |= _BV(directoryNumber % 8);
        ++folderCount;
      }
      ++directoryNumber;
      dir = SD.open("/");
      if (!dir || !dir.seek(resume)) break;
    }
    entry.close();
  }
  dir.close();
}

void hashCatalogEntry(File& entry) {
  const char* name = entry.name();
  do {
    catalogSignature = (catalogSignature ^ uint8_t(*name)) * 16777619UL;
  } while (*name++);
  uint32_t size = entry.isDirectory() ? 0 : entry.size();
  for (uint8_t i = 0; i < 4; ++i) {
    catalogSignature = (catalogSignature ^ uint8_t(size)) * 16777619UL;
    size >>= 8;
  }
}

bool selectFolder(uint16_t index) {
  if (!index) strcpy(folder, "/");
  else {
    File dir = SD.open("/");
    dir.rewindDirectory();
    uint16_t seen = 0;
    uint16_t directoryNumber = 0;
    bool found = false;
    while (true) {
      File entry = dir.openNextFile();
      if (!entry) break;
      if (entry.isDirectory()) {
        bool eligible = directoryNumber < 256 &&
          (playableFolders[directoryNumber / 8] & _BV(directoryNumber % 8));
        ++directoryNumber;
        if (eligible && ++seen == index) {
          snprintf(folder, sizeof(folder), "/%s", entry.name());
          found = true;
        }
      }
      entry.close();
      if (found) break;
    }
    dir.close();
    if (!found) return false;
  }
  folderIndex = index;
  trackCount = countTracks(folder);
  trackIndex = 0;
  Serial.print(F("FOLDER ")); Serial.print(folder);
  Serial.print(F(" index=")); Serial.print(folderIndex + 1);
  Serial.print('/'); Serial.print(folderCount);
  Serial.print(F(" files=")); Serial.println(trackCount);
  return true;
}

bool startTrack(uint16_t index) {
  stopPlayback();
  stopped = false;
  trackIndex = index;
  song[0] = 0;
  File dir = SD.open(folder);
  dir.rewindDirectory();
  uint16_t seen = 0;
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) break;
    bool selected = isPsg(entry) && seen++ == index;
    if (selected) strncpy(song, entry.name(), sizeof(song) - 1);
    entry.close();
    if (selected) break;
  }
  dir.close();
  if (!song[0]) { statusText = F("No PSG files"); return false; }
  char path[28];
  snprintf(path, sizeof(path), "%s%s%s", folder, folderIndex ? "/" : "", song);
  music = SD.open(path);
  uint8_t header[16];
  if (!music || music.read(header, sizeof(header)) != 16 ||
      header[0] != 'P' || header[1] != 'S' || header[2] != 'G' || header[3] != 0x1A) {
    music.close();
    statusText = F("Bad PSG header");
    Serial.print(F("Invalid PSG: ")); Serial.println(path);
    return false;
  }
  uint8_t rate = header[4] >= 10 && header[5] ? header[5] : 50;
  if (rate > 100 || rate < 25) {
    music.close(); statusText = F("Bad PSG rate"); return false;
  }
  // No duration scan: start after reading the header and initial buffer only.
  dataSize = music.size() - 16;
  // Reset only with playback stopped. Timer0 and Wire interrupts stay enabled.
  digitalWrite(2, LOW); delay(2); digitalWrite(2, HIGH);
  for (uint8_t reg = 0; reg < 14; ++reg) ayWrite(reg, 0);
  playbackPrepared = true;
  fillBuffer();
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    OCR1A = F_CPU / 256UL / rate - 1;
    TCNT1 = 0;
    TIFR1 = _BV(OCF1A);
    playing = true;
  }
  statusText = F("Playing");
  Serial.print(F("PLAY ")); Serial.print(path);
  Serial.print(F(" / ")); Serial.print(rate); Serial.println(F(" Hz"));
  return true;
}

bool moveFolder(int8_t direction) {
  stopPlayback();
  uint16_t candidate = folderIndex;
  for (uint16_t tries = 0; tries < folderCount; ++tries) {
    candidate = direction > 0 ? (candidate + 1UL) % folderCount :
      (candidate ? candidate - 1 : folderCount - 1);
    if (selectFolder(candidate) && trackCount) return true;
  }
  statusText = F("No PSG files");
  song[0] = 0;
  return false;
}

void moveTrack(int8_t direction, bool automatic) {
  if (!sdReady) return;
  if (!trackCount) {
    if (moveFolder(direction)) startTrack(0);
    return;
  }
  uint16_t next;
  if (repeatsCurrentTrack(playbackMode, automatic)) {
    startTrack(trackIndex);
    return;
  }
  if (playbackMode == Shuffle) {
    next = randomOtherTrack(trackIndex, trackCount, trackCount > 1 ? random(trackCount - 1) : 0);
  } else if (direction > 0) {
    next = trackIndex + 1;
    if (next >= trackCount) { if (!moveFolder(1)) return; next = 0; }
  } else {
    if (!trackIndex) { if (!moveFolder(-1)) return; next = trackCount - 1; }
    else next = trackIndex - 1;
  }
  startTrack(next);
}

void saveBookmark() {
  if (!sdReady || !song[0] || !trackCount || (!music && !stopped)) return;
  PlayerBookmark b = {};
  b.marker = 0xA7; b.version = 1; b.catalog = catalogSignature;
  b.folderIndex = folderIndex; b.trackIndex = trackIndex;
  b.folderCount = folderCount; b.trackCount = trackCount;
  strcpy(b.folder, folder); strcpy(b.song, song);
  b.checksum = bookmarkChecksum(b);
  // Commit marker last. A power loss during writing produces an invalid record.
  EEPROM.update(0, 0);
  const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&b);
  for (uint8_t i = 1; i < sizeof(b); ++i) {
    fillBuffer();
    EEPROM.update(i, bytes[i]);
  }
  fillBuffer(); EEPROM.update(0, b.marker);
  savedAt = millis(); saveNotice = true;
  Serial.print(F("SAVED ")); Serial.print(folder); Serial.print('/'); Serial.println(song);
}

bool restoreBookmark() {
  PlayerBookmark b;
  EEPROM.get(0, b);
  if (!bookmarkValid(b, catalogSignature, folderCount)) return false;
  if (!selectFolder(b.folderIndex) || strcmp(folder, b.folder) || trackCount != b.trackCount)
    return false;
  // Verify the name at the saved index before starting; never substitute a file.
  File dir = SD.open(folder);
  dir.rewindDirectory();
  uint16_t seen = 0;
  bool matches = false;
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) break;
    bool selected = isPsg(entry) && seen++ == b.trackIndex;
    if (selected) matches = strcmp(entry.name(), b.song) == 0;
    entry.close();
    if (selected) break;
  }
  dir.close();
  if (!matches) return false;
  Serial.println(F("BOOKMARK restored"));
  // Return before opening the song: bookmark and directory locals consume
  // scarce Nano stack space needed by SD.open/openNextFile.
  trackIndex = b.trackIndex;
  return true;
}

uint8_t glyphColumn(char ch, uint8_t column) {
  if (column >= 5 || ch < 32 || uint8_t(ch) > 127) return 0;
  return pgm_read_byte(Adafruit5x7 + 6 + uint16_t(uint8_t(ch) - 32) * 5 + column) & 0x7F;
}

uint8_t footerTextPixel(uint8_t x) {
  if (x < 12 && screenMode < 3)
    return (pgm_read_byte(&modeSymbols[screenMode][x]) >> 6) & 1;
  if (x >= 18 && x < 102) return glyphColumn(screenText[2][x / 6], x % 6) >> 6;
  return 0;
}

void drawFooter(uint8_t first, uint8_t last, uint8_t width) {
  oled.setCursor(first, 3);
  for (uint8_t x = first; x <= last; ++x)
    oled.ssd1306WriteRam(footerDisplayColumn(x, width, footerTextPixel(x)));
}

void updateText(uint8_t row, const char* text, uint8_t start = 0) {
  uint8_t length = strlen(text);
  for (uint8_t col = start; col < 17; ++col) {
    char value = col - start < length ? text[col - start] : ' ';
    if (screenText[row][col] == value) continue;
    oled.setCursor(col * 6, row);
    screenText[row][col] = value;
    // Text begins at pixels 0, 9, 18: two empty lines between text rows.
    for (uint8_t x = 0; x < 6; ++x)
      oled.ssd1306WriteRam(textPageColumn(glyphColumn(value, x), row));
    if (row == 2) drawFooter(col * 6, col * 6 + 5, screenBar == 255 ? 0 : screenBar);
    fillBuffer();
  }
}

void drawDisplay() {
  if (!displayReady) return;
  uint32_t position;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { position = consumed; }
  char line[22];
  uint8_t icon = paused ? 1 : playing && !finished ? 0 : 2;
  if (icon != screenIcon) {
    for (uint8_t page = 0; page < 3; ++page) {
      oled.setCursor(104, page);
      for (uint8_t x = 0; x < 24; ++x)
        oled.ssd1306WriteRam(transportIconColumn(icon, x, page));
      fillBuffer();
    }
    screenIcon = icon;
  }
  // Folder name and its index/count have a full row of their own.
  folderDisplayLine(line, folderIndex, folderCount, folder);
  // Reserve the last three text cells for channel order; the icon starts x104.
  for (uint8_t i = strlen(line); i < 14; ++i) line[i] = ' ';
  for (uint8_t i = 0; i < 3; ++i) line[14 + i] = pgm_read_byte(&channelLabels[channelMode][i]);
  line[17] = 0;
  updateText(0, line);
  updateText(1, song);
  if (screenMode != playbackMode) {
    oled.setCursor(0, 2);
    for (uint8_t i = 0; i < 12; ++i)
      oled.ssd1306WriteRam((pgm_read_byte(&modeSymbols[playbackMode][i]) & 0x7F) << 2);
    screenMode = playbackMode;
    drawFooter(0, 11, screenBar == 255 ? 0 : screenBar);
  }
  if (saveNotice && millis() - savedAt >= 1500) saveNotice = false;
  if (saveNotice) strcpy(line, "Saved");
  else if (playing || paused || stopped) {
    uint8_t percent = dataSize ? uint8_t(min(100UL, uint32_t(uint64_t(position) * 100 / dataSize))) : 0;
    trackDisplayLine(line, trackIndex, trackCount, percent);
  } else {
    strncpy_P(line, reinterpret_cast<PGM_P>(statusText), sizeof(line) - 1);
    line[sizeof(line) - 1] = 0;
  }
  updateText(2, line, 3);
  uint8_t width = insetBarWidth(position, dataSize);
  if (width != screenBar) {
    uint8_t first = screenBar == 255 ? 0 : min(width, screenBar) + 3;
    uint8_t last = screenBar == 255 ? 127 : max(width, screenBar) + 2;
    drawFooter(first, last, width);
    screenBar = width;
    fillBuffer();
  }
}

void setup() {
  statusText = F("Starting");
  Serial.begin(9600);
  Serial.println(F("AYNano ZX Player " AY_PLAYER_VERSION " / 6 keys"));
  for (uint8_t pin = A0; pin <= A3; ++pin) pinMode(pin, OUTPUT);
  for (uint8_t pin = 2; pin <= 10; ++pin) pinMode(pin, OUTPUT);
  digitalWrite(8, LOW); digitalWrite(9, LOW); digitalWrite(10, HIGH);
  digitalWrite(2, LOW);
  TCCR2A = 0x23; TCCR2B = 0x09; OCR2A = 8; OCR2B = 3;
  delay(2); digitalWrite(2, HIGH); silence();
  Wire.begin(); Wire.setClock(100000UL); Wire.setWireTimeout(25000, true);
  for (uint8_t address = 0x3C; address <= 0x3D; ++address) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) {
      oled.begin(&Adafruit128x32, address); oled.setFont(Adafruit5x7); oled.clear();
      displayReady = true;
      Serial.print(F("OLED 0x")); Serial.println(address, HEX);
      break;
    }
  }
  if (displayReady) {
    oled.set1X(); oled.setCursor(16, 0); oled.print(F("AYNano ZX Player"));
    oled.set1X(); oled.setCursor(36, 2); oled.print(F("v" AY_PLAYER_VERSION));
    oled.setCursor(18, 3); oled.print(F("Scanning SD..."));
  }
  randomSeed(analogRead(A7) ^ micros());
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    TCCR1A = 0; TCCR1B = _BV(WGM12) | _BV(CS12); TCNT1 = 0;
    OCR1A = F_CPU / 256UL / 50 - 1; TIMSK1 = _BV(OCIE1A);
  }
  sdReady = SD.begin(10);
  if (!sdReady) { statusText = F("SD error"); if (displayReady) oled.clear(); drawDisplay(); return; }
  scanFolders();
  if (displayReady) oled.clear(); // clear startup before audio begins
  if (restoreBookmark()) {
    startTrack(trackIndex);
  } else {
    selectFolder(0);
    if (trackCount || moveFolder(1)) startTrack(0);
    else statusText = F("No PSG files");
  }
  drawDisplay();
}

void loop() {
  fillBuffer();
  char command = Serial.available() ? Serial.read() : 0;
  if (command == 'D') {
    uint16_t restoreFolder = folderIndex, restoreTrack = trackIndex;
    stopPlayback();
    for (uint16_t i = 0; i < folderCount; ++i) {
      if (!selectFolder(i)) Serial.println(F("FOLDER failed"));
    }
    // Exercise the same next/previous operations as physical keys 6 and 5.
    if (moveFolder(1)) startTrack(0);
    if (moveFolder(-1)) startTrack(0);
    if (selectFolder(restoreFolder)) startTrack(restoreTrack);
  }
  uint16_t raw = analogRead(A6);
  uint8_t reading = raw >= 950 ? 0 : ayButtonId(raw);
  if (!reading && raw < 950) reading = 255;
  uint8_t button = keys.update(reading, millis());
  if (button) {
    Serial.print(F("KEY ")); Serial.println(button);
    switch (button) {
      case 1: moveTrack(-1, false); break;
      case 2: moveTrack(1, false); break;
      case 3:
        if (stopped) startTrack(trackIndex);
        else if (music && !finished) {
          paused = !paused;
          ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { playing = false; }
          if (paused) silence();
          else {
            for (uint8_t i = 0; i < 3; ++i) ayWrite(8 + i, levels[i]);
            ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
              TCNT1 = 0; TIFR1 = _BV(OCF1A); playing = true;
            }
          }
        }
        break;
      case 4:
        playbackMode = nextPlaybackMode(playbackMode);
        Serial.print(F("MODE ")); Serial.println(uint8_t(playbackMode));
        break;
      case 5: if (sdReady && moveFolder(-1)) startTrack(0); break;
      case 6: if (sdReady && moveFolder(1)) startTrack(0); break;
      case 13:
        if (song[0]) { stopPlayback(); stopped = true; statusText = F("Stopped"); }
        break;
      case 14: saveBookmark(); break;
      case 16: cycleChannelMode(); break;
    }
    lastDisplay = millis() - 250;
  }
  if (finished && !paused) moveTrack(1, true);
  if (millis() - lastDisplay >= 250) { lastDisplay = millis(); drawDisplay(); }
}
