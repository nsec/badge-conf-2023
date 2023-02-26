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

// SOFTWARE SERIAL
SoftwareSerial g_rightSerial(SIG_R2, SIG_R1); // RX, TX
SoftwareSerial g_leftSerial(SIG_L1, SIG_L2); // RX, TX

// PROTOTYPE FUNCTION
void softSerialRoutine();
void incrementLoadingBar();

// VARIABLE
#define MAX_MESSAGE_LENGTH 8

void setup()
{
	nsec::g::the_badge.setup();

	g_rightSerial.begin(4800);
	g_leftSerial.begin(4800);

	UniqueIDdump(Serial);

	Serial.println(F("Setup completed"));
}

void loop()
{
	softSerialRoutine();

	// NSEC COMMUNICATION
	const bool commLeftConnected = digitalRead(SIG_L3) == LOW;
	const bool commRightConnected = digitalRead(SIG_R2) == LOW;

	nsec::g::the_scheduler.tick(millis());
}

// FUNCTION DECLARATION
void softSerialRoutine()
{
	static bool pingpongSelector = false;
	static uint32_t sending = 0;
	static char receivedRight[MAX_MESSAGE_LENGTH];
	static char receivedLeft[MAX_MESSAGE_LENGTH];

	static uint32_t ts_loop = 0;
	if (millis() - ts_loop > 1000) {
		ts_loop = millis();

		sending = millis();
		g_leftSerial.println(sending);
		g_rightSerial.println(sending);
	}

	static uint32_t ts_pingpong = 0;
	if (millis() - ts_pingpong > 200) {
		ts_pingpong = millis();
		pingpongSelector = !pingpongSelector;
	}

	static char msgRxRight[MAX_MESSAGE_LENGTH];
	static unsigned int msgRxRight_pos = 0;
	static char msgRxLeft[MAX_MESSAGE_LENGTH];
	static unsigned int msgRxLeft_pos = 0;
	if (!pingpongSelector) {
		g_rightSerial.listen();
		while (g_rightSerial.available() > 0) {
			char const inByte = g_rightSerial.read();
			if (inByte != '\n' && (msgRxRight_pos < MAX_MESSAGE_LENGTH - 1)) {
				msgRxRight[msgRxRight_pos] = inByte; // Add the incoming byte to our
								     // message
				msgRxRight_pos++;
			} else {
				msgRxRight[msgRxRight_pos] = '\0'; // Add null character to string
				msgRxRight_pos = 0; // Reset for the next message
				for (int i = 0; i < MAX_MESSAGE_LENGTH; i++) {
					receivedRight[i] = msgRxRight[i];
				}
			}
		}
	} else {
		g_leftSerial.listen();
		while (g_leftSerial.available() > 0) {
			char const inByte = g_leftSerial.read();
			if (inByte != '\n' && (msgRxLeft_pos < MAX_MESSAGE_LENGTH - 1)) {
				msgRxLeft[msgRxLeft_pos] = inByte; // Add the incoming byte to our
								   // message
				msgRxLeft_pos++;
			} else {
				msgRxLeft[msgRxLeft_pos] = '\0'; // Add null character to string
				msgRxLeft_pos = 0; // Reset for the next message
				for (int i = 0; i < MAX_MESSAGE_LENGTH; i++) {
					receivedLeft[i] = msgRxLeft[i];
				}
			}
		}
	}
}
