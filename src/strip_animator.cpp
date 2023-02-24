/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2023 Jérémie Galarneau <jeremie.galarneau@gmail.com>
 */

#include "board.hpp"
#include "globals.hpp"
#include "led/strip_animator.hpp"

namespace nl = nsec::led;
namespace ns = nsec::scheduling;
namespace ng = nsec::g;

namespace {
constexpr ns::relative_time_ms initial_refresh_period_ms = 200;
}

nl::strip_animator::strip_animator() noexcept :
	ns::periodic_task(initial_refresh_period_ms),
	_pixels(NUMPIXELS, P_NEOP, NEO_GRB + NEO_KHZ800)
{
	ng::the_scheduler.schedule_task(*this);
}

void nl::strip_animator::setup() noexcept
{
	_pixels.begin();
}

void nl::strip_animator::run(scheduling::absolute_time_ms current_time_ms) noexcept
{
	if (_start_position > 15) {
		// wrap around when reaching the end led strip
		_start_position = 0;
	}

	// Set all pixel colors to 'off'
	_pixels.clear();
	for (unsigned int i = 0; i < _level; i++) // determine how many LED should be ON
	{
		// led_ID is the current LED index that we are update.
		uint8_t const led_ID = (_start_position + i) % 16; // This is going over every
								   // single LED that needs to
								   // be on based on the
								   // current LVL

		// enable more colors if your lvl is more than the 16 LEDs
		uint8_t b = 0;
		uint8_t r = 0;
		uint8_t g = 0;
		if (i < 16) // only blue to start
		{
			b = 5;
		} else if (i >= 16 && i < 32) // introduce red
		{
			b = 5;
			r = 5;
		} else if (i >= 32 && i < 48) // introduce green
		{
			b = 5;
			g = 5;
		} else if (i >= 48 && i < 64) // have all 3 colors
		{
			g = 5;
			r = 5;
			b = 5;
		} else if (i >= 64 && i < 80) // increase brightness + speed
		{
			period_ms(100);
			b = 10;
			r = 5;
			b = 5;
		} else if (i >= 80 && i < 96) // increase brightness
		{
			period_ms(50);
			b = 10;
			r = 10;
			b = 5;
		} else if (i >= 96 && i < 112) // increase brightness
		{
			period_ms(25);
			b = 10;
			r = 10;
			b = 10;
		} else if (i >= 112 && i < 128) // max score
		{
			period_ms(12);
			r = random(1, 10);
			g = 0;
			b = 0;
		} else if (i >= 128 && i < 144) // max score
		{
			period_ms(6);
			r = random(1, 10);
			g = 0;
			b = random(1, 10);
		} else if (i >= 144 && i < 160) // max score
		{
			period_ms(3);
			r = random(1, 10);
			g = random(1, 10);
			b = random(1, 10);
		} else if (i >= 160) // max score
		{
			period_ms(1);
			r = random(1, 15);
			g = random(1, 15);
			b = random(1, 15);
		}

		// apply color
		_pixels.setPixelColor(led_ID, _pixels.Color(r, g, b));
	}

	_start_position++;
	// Send the updated pixel colors to the hardware.
	_pixels.show();
}

void nl::strip_animator::set_current_animation_idle(unsigned int current_level) noexcept
{
	_start_position = 0;
	_level = current_level;
}
