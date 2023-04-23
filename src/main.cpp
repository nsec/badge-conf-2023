// SPDX-FileCopyrightText: 2023 NorthSec
//
// SPDX-License-Identifier: MIT

#include "board.hpp"
#include "globals.hpp"

#include <Arduino.h>
#include <ArduinoUniqueID.h>
#include <SPI.h>
#include <SoftwareSerial.h>
#include <Wire.h>

void setup()
{
	nsec::g::the_badge.setup();

	UniqueIDdump(Serial);
	Serial.println(F("Setup completed"));
}

void loop()
{
	nsec::g::the_scheduler.tick(millis());
}
