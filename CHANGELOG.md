# AYNano ZX Player releases

## 1.7 — 2026-10-10

- Remove the surrounding frame from the large play/pause/stop symbols.
- Animate a small wave beneath the title while the SD catalogue is scanned.
- Give button actions a brief transport-symbol pulse and reveal new track names over 400 ms without delaying playback.
- Move the diagonal hatching inside the progress bar while playing; freeze it on pause/stop. Keep the thin frame, text gap and actual progress endpoint.
- Generate animation pixels directly with six additional bytes of static RAM; refill the music buffer during footer redraws. EEPROM bookmarks and controls retain their format/behavior.
- Close the current song before the Serial folder diagnostic restores the original folder, keeping SD enumeration to two live handles.

Validation: 186 display and 19 animation assertions passed on compiled AVR code; existing 25 routing, 55 control/bookmark and 12 timing checks also passed. Firmware: 27176 bytes flash, 1488 bytes static RAM. COM6 flash verification succeeded. Hardware checks found all 12 music sources, restored the bookmark and played songs across folder transitions; the music buffer reported zero underruns during animation. Release files and the hardware log are in `releases/1.7`.

## 1.6 — 2026-10-10

- Rename the player to AYNano ZX Player on the startup screen and Serial log.
- Fix startup failing to open a PSG after restoring an EEPROM bookmark: return from bookmark validation before opening the song, releasing its local stack storage.
- Reduce the screen cache to its 17 visible columns, recovering 15 bytes of static RAM. Channel allocations and EEPROM format are unchanged.
- Firmware: 26184 bytes flash, 1482 bytes static RAM. COM6 upload and flash verification succeeded; saved LYRA3.PSG started successfully after reset.

## 1.5 — 2026-10-10

- Hold button 6 for one second to cycle ABC, ACB, BAC, BCA, CAB, CBA.
- Short button 6 action selects the next folder on release.
- Channel allocation shown at the right of the top text line.
- Tone, volume and tone/noise mixer bits are routed together. Changing allocation reapplies current state without restarting playback or retriggering the shared envelope; paused/stopped playback stays silent.
- Allocation starts at ABC on power-up; EEPROM bookmarks retain their existing format.

Validation: 25 routing assertions (including all six allocations and all 256 mixer values) and 55 control assertions passed against compiled AVR code. Firmware size: 26170 bytes flash, 1497 bytes static RAM. Upload to COM6 and flash verification succeeded. Release files are in `releases/1.5`.

## 1.4 — 2026-10-10

- Startup screen shows AY Player and v1.4 while the SD catalogue is scanned.
- Large 24×24 play/pause/stop indicator in the top right corner.
- Playback modes cycle through sequential, shuffle and repeat one track.
- Repeat applies at the end of a track; manual next/previous still changes tracks.
- Existing folder controls, stop, EEPROM bookmark and thin hatched progress bar retained.

Earlier development builds were unversioned. Future firmware updates increment the minor version.

Validation: 176 display assertions and 49 control/bookmark assertions passed against compiled AVR code. Firmware size: 25486 bytes flash, 1482 bytes static RAM. Upload to COM6 and flash verification succeeded. The release HEX and startup log are in `releases/1.4`.
