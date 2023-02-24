// SPDX-FileCopyrightText: 2023 NorthSec
//
// SPDX-License-Identifier: MIT

#include "board.hpp"
#include "display.hpp"
#include "globals.hpp"
#include "neopixel.hpp"

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
	// GPIO INIT
	pinMode(LED_DBG, OUTPUT);

	/* Buttons */
	pinMode(BTN_RIGHT, INPUT_PULLUP);
	pinMode(BTN_UP, INPUT_PULLUP);
	pinMode(BTN_DOWN, INPUT_PULLUP);
	pinMode(BTN_LEFT, INPUT_PULLUP);

	/* Software serial */
	pinMode(SIG_R2, INPUT_PULLUP);
	pinMode(SIG_R3, OUTPUT);
	digitalWrite(SIG_R3, LOW);

	pinMode(SIG_L2, OUTPUT);
	digitalWrite(SIG_L2, LOW);
	pinMode(SIG_L3, INPUT_PULLUP);

	// SERIAL INIT
	Serial.begin(38400);

	g_rightSerial.begin(4800);
	g_leftSerial.begin(4800);

	// NEOPIXEL INIT
	neopix_init();

	// DISPLAY INIT
	nsec::display::init();

	// OLED INIT
	g_display.setTextColor(SSD1306_WHITE);
	g_display.clearDisplay();
	g_display.setTextSize(1);
	/*
	g_display.print(F("John Smith"));
	g_display.startscrollleft(0x00, 0x0F);
	*/
	g_display.display(); // Show initial text

	UniqueIDdump(Serial);

	Serial.println(F("Setup completed"));
}

void loop()
{
	softSerialRoutine();

	// NSEC COMMUNICATION
	const bool commLeftConnected = digitalRead(SIG_L3) == LOW;
	const bool commRightConnected = digitalRead(SIG_R2) == LOW;

	//--------------------------------------------
	// DEBUG OUTPUT
	static uint32_t ts_uart = 0;
	if (millis() - ts_uart > 100) {
		ts_uart = millis();

		// cheat code
		if (digitalRead(BTN_UP) == LOW) {
			Serial.print("\t Level Up");

			if (nsec::g::currentLevel < 199) {
				nsec::g::currentLevel++;
			}

			g_display.clearDisplay();
			g_display.setCursor(0, 0);
			g_display.println();
			g_display.print(F("LVL="));
			g_display.print(nsec::g::currentLevel);
			g_display.display();
		} else if (digitalRead(BTN_DOWN) == LOW) {
			Serial.print("\t Level Down");

			if (nsec::g::currentLevel > 1) {
				nsec::g::currentLevel--;
			}

			g_display.clearDisplay();
			g_display.setCursor(0, 0);
			g_display.println();
			g_display.print(F("LVL="));
			g_display.print(nsec::g::currentLevel);
			g_display.display();
		}
	}

	//--------------------------------------------
	// NEOPIXEL UPDATE
	static uint32_t ts_neopix = 0;
	if (millis() - ts_neopix > 1) {
		ts_neopix = millis();
		if (commLeftConnected) {
			if (!nsec::g::waitingForDisconnect) {
				incrementLoadingBar();
				neopix_connectLeft();
			} else {
				neopix_success();
			}

		} else if (commRightConnected) {
			if (!nsec::g::waitingForDisconnect) {
				neopix_connectRight();
				incrementLoadingBar();
			} else {
				neopix_success();
			}
		} else {
			neopix_idle();
			/*
			if(nsec::g::waitingForDisconnect == true)	//if you just disconnect
			after pairing successfully. Return to showing the name
			{
				g_display.clearDisplay();
				g_display.setTextSize(2); // Draw 2X-scale text
				g_display.print(F("John Smith"));
				g_display.display();      // Show initial text
				g_display.startscrollleft(0x00, 0x0F);
			}
			*/

			nsec::g::waitingForDisconnect = false; // clear loading bar variables
			nsec::g::currentlyLoading = false;
			nsec::g::loadingBarPos = 0;
		}
	}
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

	static uint32_t ts_display = 0;
	if (millis() - ts_display > 100) {
		ts_display = millis();
		g_display.clearDisplay();
		g_display.setCursor(0, 0);
		g_display.print("broadcast:");
		g_display.println(sending);
		g_display.print("\nLeft RX:");
		g_display.println(receivedLeft);
		g_display.print("Right RX:");
		g_display.println(receivedRight);
		g_display.display();
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

/*
 * Loading bar for connection, wait for 8 seconds?
 */
void incrementLoadingBar()
{
	uint32_t ts_loadingStartTime = 0;
	uint32_t ts_loadingNextIncrement = 0;

	if (!nsec::g::currentlyLoading) {
		nsec::g::currentlyLoading = true;
		ts_loadingStartTime = millis();
		ts_loadingNextIncrement = ts_loadingStartTime + 1000;
		g_pixels.fill(g_pixels.Color(0, 0, 0), 8, 15); // clear bottom row
		g_pixels.setPixelColor(15, g_pixels.Color(30, 0, 30));
		g_pixels.show();
	}

	if ((millis() > ts_loadingNextIncrement) && (!nsec::g::waitingForDisconnect)) {
		if (nsec::g::loadingBarPos < 8) {
			ts_loadingNextIncrement = millis() + 1000; // next increment is in 1 seconds
			nsec::g::loadingBarPos++;
			g_pixels.fill(g_pixels.Color(0, 0, 0), 8, 15); // clear bottom row
			g_pixels.fill(g_pixels.Color(30, 0, 30),
				      15 - nsec::g::loadingBarPos + 1,
				      nsec::g::loadingBarPos);
			g_pixels.show();
		} else {
			nsec::g::currentLevel++;
			if (nsec::g::currentLevel >= 200)
				nsec::g::currentLevel = 199; // cap level at 199. No more additional
							     // LED animation after 199
			nsec::g::waitingForDisconnect = true;
		}
	}
}
