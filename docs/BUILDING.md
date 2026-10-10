# Сборка AYNano ZX Player

## Arduino IDE

1. Установите Arduino IDE и пакет **Arduino AVR Boards 1.8.8**.
2. Скопируйте папки `SD` и `SSD1306Ascii` из `LIB/libraries` в каталог `libraries` вашего Arduino sketchbook. Используются версии **SD 1.3.0** и **SSD1306Ascii 1.1.3**.
3. Откройте `firmware/AYPlayerNano/AYPlayerNano.ino`.
4. Выберите **Arduino Nano**, процессор **ATmega328P (Old Bootloader)** и порт своей платы.
5. Проверьте скетч и загрузите прошивку.

Эта конкретная Nano подключалась через COM6. На другом компьютере порт будет другим. Для Nano с новым загрузчиком следует выбрать соответствующий процессор в IDE.

## Arduino CLI

При установленном `arduino-cli` команды выполняются из корня репозитория:

```powershell
arduino-cli core update-index
arduino-cli core install arduino:avr@1.8.8
arduino-cli compile --fqbn arduino:avr:nano:cpu=atmega328old --libraries ./LIB/libraries --build-path ./build/player ./firmware/AYPlayerNano
arduino-cli upload --fqbn arduino:avr:nano:cpu=atmega328old --port COM6 --input-dir ./build/player
```

Прошивка использует калибровку из `firmware/AYPlayerNano/ButtonCalibration.h`. Если меняете номиналы лестницы, измерьте значения A6 и обновите центры распознавания кнопок.

## Особенность сброса проверенной платы

У этой Nano стандартный автоматический сброс avrdude не обеспечивал синхронизацию. Работала последовательность: открыть порт, установить DTR/RTS на 100 мс, снять сигналы, закрыть порт и запустить **avrdude 8.0.0** с `-x noautoreset`, протоколом `arduino` и скоростью **57600**.

`build-player.ps1` сохраняет эту проверенную последовательность. Скрипт рассчитан на локальную установку инструментов в соседнем каталоге `../AY-PLAYER/.tools` и `../AY-PLAYER/.arduino`; эти инструменты не входят в репозиторий. На других компьютерах используйте IDE/CLI выше или адаптируйте пути скрипта.

```powershell
./build-player.ps1
./build-player.ps1 -Upload -Port COM6
```

Скрипт перед сборкой копирует калибровку из `diagnostics/ButtonCalibration.h` в каталог прошивки. Поэтому при использовании скрипта обновляйте обе копии. Параметр avrdude `-D` сохраняет EEPROM; скрипт записывает и проверяет только flash.

## Готовый HEX

`releases/1.7/AYNano-ZX-Player-1.7.hex` предназначен для **ATmega328P 16 МГц** с описанным подключением. Это образ приложения, без загрузчика и EEPROM. Загружайте его подходящим AVR-инструментом без стирания сохранённой EEPROM-закладки.

## Проверки

В `tests` находятся пять автономных наборов: `timing.cpp`, `controls.cpp`, `display.cpp`, `routing.cpp`, `animation.cpp`. Они компилируются `avr-g++` для ATmega328P, переводятся в бинарный файл и выполняются симулятором **avr8js 0.21.0** через Node.js.

Для установки симулятора:

```powershell
npm install
```

Пример одного набора (инструменты AVR должны быть доступны в PATH):

```powershell
avr-g++ -mmcu=atmega328p -std=gnu++11 -Os tests/routing.cpp -o tests/routing.elf
avr-objcopy -O binary tests/routing.elf tests/routing.bin
avr-nm tests/routing.elf | Out-File -Encoding ascii tests/routing.symbols
node tests/run-timing.cjs routing
```

Повторите для `timing`, `controls`, `display` и `animation`. Эти проверки подтверждают логику модулей, но не эмулируют реальную SD-карту и всю собранную плату.

Последняя проверенная сборка: **27176 байт flash, 1488 байт статической RAM**. При изменениях следите за памятью: Nano имеет всего 2 КБ RAM, а SD использует и стек, и динамически выделяемые объекты.

Serial, 9600 бод: команда `S` выводит счётчик `underruns` (кадры, которым не хватило данных) и число исполненных музыкальных байтов. Команда `D` проверяет список папок, переходы вперёд/назад и возвращает исходный выбранный трек.


