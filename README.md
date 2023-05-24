<!--
SPDX-FileCopyrightText: 2023 NorthSec

SPDX-License-Identifier: MIT
-->

Nsec 2023 Conference Badge Firmware
===================================

## Hardware

The 2023 Conference badge is loosely based on the Arduino UNO design with an
[ATmega328PB](https://en.wikipedia.org/wiki/ATmega328) microcontroller and the
following devices:

- Sixteen [NeoPixel](https://en.wikipedia.org/wiki/Adafruit_Industries#NeoPixel) RGB LEDs
- Six buttons
- Two 'pairing' connectors
- One [Shitty Add-On V1.69bis](https://hackaday.com/2019/03/20/introducing-the-shitty-add-on-v1-69bis-standard/) connector
- One (optional) 128x32 OLED display

The badge is powered through a USB-C port or through 3 AAA batteries.


## Building

The firmware is based on the [Arduino
framework](https://docs.platformio.org/en/stable/frameworks/arduino.html) using
the [MiniCore](https://github.com/MCUdude/MiniCore) core. The build system uses
[PlatformIO](https://docs.platformio.org/en/stable/what-is-platformio.html) to
easily manage the dependencies.

You can install it on Debian / Ubuntu in a python virtualenv with these
commands:

```bash
sudo apt install python3-virtualenv

virtualenv .venv
. .venv/bin/activate

pip install platformio
```

The installation procedure for your OS may differ a little, please consult the
[PlatformIO
documentation](https://docs.platformio.org/en/stable/core/installation/index.html)
if you have any difficulties.

Once the installation is complete you can build the firmware:

```bash
pio run
```


## Flashing

Flashing the firmware requires an AVR compatible programmer, the cheapest
available one we could find is the [USBasp](https://www.fischl.de/usbasp/)
which can be found for 10$ to 20$ on Amazon or other retailers.

If the badge is factory fresh, you need to set the fuses once with this
command:

```bash
pio run -t fuses
```

Then flash the firmware with this command:

```bash
pio run -t upload
```

## Pre-built firmware

If you don't want to make any modifications and just want to flash the firmware
that was used during the conference, you just need to install
[AVRDUDE](https://www.nongnu.org/avrdude/) and flash the 'binary/firmware.hex'
file.

On Debian / Ubuntu:

```bash
sudo apt install avrdude

# Set the fuses
avrdude -e -p atmega328pb -c usbasp -B 10 -U lock:w:0x0f:m -U hfuse:w:0xde:m -U lfuse:w:0xff:m -U efuse:w:0xfd:m

# Flash the firmware
avrdude -e -p atmega328pb -c usbasp -B 3Mhz -U "flash:w:binary/firmware.hex:i"
```

Enjoy.
