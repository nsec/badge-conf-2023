/*
 * Copyright 2023 Jérémie Galarneau <jeremie.galarneau@gmail.com>
 */

#include "badge.hpp"
#include "board.hpp"
#include "globals.hpp"

#include <Arduino.h>

void nsec::runtime::badge::setup()
{
	// GPIO INIT
	pinMode(LED_DBG, OUTPUT);

	// Buttons
	pinMode(BTN_RIGHT, INPUT_PULLUP);
	pinMode(BTN_UP, INPUT_PULLUP);
	pinMode(BTN_DOWN, INPUT_PULLUP);
	pinMode(BTN_LEFT, INPUT_PULLUP);

	// Init software serial for both sides of the badge.
	pinMode(SIG_R2, INPUT_PULLUP);
	pinMode(SIG_R3, OUTPUT);
	digitalWrite(SIG_R3, LOW);

	pinMode(SIG_L2, OUTPUT);
	pinMode(SIG_L3, INPUT_PULLUP);
        digitalWrite(SIG_L2, LOW);

	// Hardware serial init (through USB-C connector)
	Serial.begin(38400);
}
