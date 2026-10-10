#include <Wire.h>
#include <SSD1306Ascii.h>
#include <SSD1306AsciiWire.h>

SSD1306AsciiWire oled;
bool found = false;
unsigned long lastSample = 0;

void setup() {
  Serial.begin(9600);
  Serial.println(F("AY Nano diagnostics: I2C A4/A5, buttons A6"));
  // Keep AY silent. Do not drive its data bus or SD card.
  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);
  Wire.begin();
  Wire.setClock(100000UL);
  Wire.setWireTimeout(25000, true);
  for (byte address = 1; address < 127; ++address) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) {
      Serial.print(F("I2C address: 0x"));
      Serial.println(address, HEX);
      if (!found && (address == 0x3C || address == 0x3D)) {
        oled.begin(&Adafruit128x32, address);
        oled.setFont(Adafruit5x7);
        oled.clear();
        oled.println(F("AY NANO DIAGNOSTICS"));
        oled.println(F("OLED 128x32 / A4 A5"));
        found = true;
      }
    }
  }
  if (!found) Serial.println(F("No OLED at 0x3C/0x3D. Check power and wiring."));
}

void loop() {
  if (millis() - lastSample < 200) return;
  lastSample = millis();
  int raw = analogRead(A6);
  Serial.print(F("A6="));
  Serial.println(raw);
  if (found) {
    oled.setCursor(0, 2);
    oled.print(F("Buttons A6: "));
    oled.print(raw);
    oled.print(F("    "));
    oled.setCursor(0, 3);
    oled.print(F("Press each button"));
  }
}
