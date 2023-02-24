// SPDX-FileCopyrightText: 2023 NorthSec
//
// SPDX-License-Identifier: MIT

#include "board.hpp"
#include "neopixel.hpp"
#include "globals.hpp"

static uint32_t g_idle_refreshInterval = 200;

Adafruit_NeoPixel g_pixels(NUMPIXELS, P_NEOP, NEO_GRB + NEO_KHZ800);

void neopix_init()
{
	g_pixels.begin();
}

void neopix_success()
{
	static uint32_t ts_lastRefresh = 0;

	if(millis() - ts_lastRefresh > 10)
	{
		ts_lastRefresh = millis();
		g_pixels.clear(); // Set all pixel colors to 'off'
		for(int i=0; i<16; i++)
		{
			g_pixels.setPixelColor(i, g_pixels.Color(random(0,10), random(0,10), random(0,10)));
		}
		g_pixels.show();   // Send the updated pixel colors to the hardware.
	}
}

void neopix_connectRight()
{
	static uint32_t ts_lastRefresh = 0;
	static int i = 0;

	if (millis() - ts_lastRefresh > 25)
	{
		ts_lastRefresh = millis();

		i++;
		if (i == 7) {
			i = 0;
		}

		g_pixels.fill(g_pixels.Color(0,0,0), 0, 8);	//clear top row
		g_pixels.setPixelColor(i, g_pixels.Color(10,0,0));
		g_pixels.show();
	}
}

void neopix_connectLeft()
{
	static uint32_t ts_lastRefresh = 0;
	static int i = 0;

	if (millis() - ts_lastRefresh > 25)
	{
		ts_lastRefresh = millis();

		i++;
		if (i == 7) {
			i=0;
		}

		g_pixels.fill(g_pixels.Color(0,0,0), 0, 8);	//clear top row
		g_pixels.setPixelColor(7-i, g_pixels.Color(10,0,0));
		g_pixels.show();
	}
}


void neopix_idle()
{
	static uint32_t ts_slowIdle = 0;
	static uint8_t startPosition = 0;
	if(millis() - ts_slowIdle > g_idle_refreshInterval)	//slow down animation
	{
		ts_slowIdle = millis();
		startPosition++;

		if(startPosition > 15)	//wrap around when reaching the end led strip
			startPosition = 0;

		g_pixels.clear(); // Set all pixel colors to 'off'
		for(int i = 0; i < nsec::g::currentLevel; i++)		//determine how many LED should be ON
		{
			//led_ID is the current LED index that we are update.
			uint8_t led_ID = (startPosition+i) % 16; //This is going over every single LED that needs to be on based on the current LVL

			/*
			if(led_ID > 15)
				led_ID = i;	//wrap around
			*/

			//enable more colors if your lvl is more than the 16 LEDs
			uint8_t b = 0;
			uint8_t r = 0;
			uint8_t g = 0;
			if (i < 16)			//only blue to start
			{
				b = 5;
			}
			else if(i >= 16 && i < 32)	//introduce red
			{
				b = 5;
				r = 5;
			}
			else if(i >= 32 && i < 48)	//introduce green
			{
				b = 5;
				g = 5;
			}
			else if(i >= 48 && i < 64)	//have all 3 colors
			{
				g=5;
				r=5;
				b=5;
			}
			else if(i >= 64 && i < 80)    //increase brightness + speed
			{
			    g_idle_refreshInterval = 100;
			    b=10;
			    r=5;
			    b=5;
			}
			else if(i >= 80 && i < 96)    //increase brightness
			{
			    g_idle_refreshInterval = 50;
			    b=10;
			    r=10;
			    b=5;
			}
			else if(i >= 96 && i < 112)    //increase brightness
			{
			    g_idle_refreshInterval = 25;
			    b=10;
			    r=10;
			    b=10;
			}
			else if(i >= 112 && i < 128)    //max score
			{
			    g_idle_refreshInterval = 12;
			    r=random(1,10);
			    g=0;
			    b=0;
			}
			else if(i >= 128 && i < 144)    //max score
			{
			    g_idle_refreshInterval = 6;
			    r=random(1,10);
			    g=0;
			    b=random(1,10);
			}
			else if(i >= 144 && i < 160)    //max score
			{
			    g_idle_refreshInterval = 3;
			    r=random(1,10);
			    g=random(1,10);
			    b=random(1,10);
			}
			else if(i >= 160)    //max score
			{
			    g_idle_refreshInterval = 1;
			    r=random(1,15);
			    g=random(1,15);
			    b=random(1,15);
			}

			//apply color
			g_pixels.setPixelColor(led_ID, g_pixels.Color(r,g,b));
		}
		g_pixels.show();   // Send the updated pixel colors to the hardware.
	}
}
