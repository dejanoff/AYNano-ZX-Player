# Firmware releases

The project is named AYNano ZX Player. The user requested visible firmware versions starting at 1.4. For each subsequent user-facing firmware update, increment the minor version (1.5, 1.6, etc.) in firmware/AYPlayerNano/FirmwareVersion.h and add a CHANGELOG.md entry. Internal rebuilds and fixes before a release use that release's version. Show AYNano ZX Player and the version on the OLED during startup and SD scanning, and log the version to Serial. Preserve the user's EEPROM bookmark when flashing updates.
