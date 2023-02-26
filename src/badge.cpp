/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2023 Jérémie Galarneau <jeremie.galarneau@gmail.com>
 */

#include "badge.hpp"
#include "board.hpp"
#include "globals.hpp"

#include <Arduino.h>

namespace nr = nsec::runtime;

nr::badge::badge() :
	_button_watcher({ [](nsec::button::id id, nsec::button::event event, void *data) {
				 auto *badge = reinterpret_cast<class badge *>(data);

				 badge->on_button_event(id, event);
			 },
			  this })
{
}

void nr::badge::setup()
{
	// GPIO INIT
	pinMode(LED_DBG, OUTPUT);

	_button_watcher.setup();
	_strip_animator.setup();
	set_social_level(1);

	// Init software serial for both sides of the badge.
	pinMode(SIG_R2, INPUT_PULLUP);
	pinMode(SIG_R3, OUTPUT);
	digitalWrite(SIG_R3, LOW);

	pinMode(SIG_L2, OUTPUT);
	pinMode(SIG_L3, INPUT_PULLUP);
	digitalWrite(SIG_L2, LOW);

	// Hardware serial init (through USB-C connector).
	Serial.begin(38400);
}

void nr::badge::on_button_event(nsec::button::id button, nsec::button::event event) noexcept
{
	const char *button_names[]{
		"Up", "Right", "Down", "Left", "Ok", "Cancel",
	};
	const char *event_names[]{ "up", "down", "down repeat" };

	Serial.printf("Button '%s' event %s\r\n",
		      button_names[static_cast<size_t>(button)],
		      event_names[static_cast<size_t>(event)]);

	if (event == nsec::button::event::UP) {
		return;
	}

	switch (button) {
	case nsec::button::id::UP:
		set_social_level(_social_level + 1);
		break;
	case nsec::button::id::DOWN:
		set_social_level(_social_level - 1);
		break;
	default:
		break;
	}
}

void nr::badge::set_social_level(uint8_t new_level)
{
	new_level = max(1, new_level);
	new_level = min(nsec::config::social::max_level, new_level);

	_social_level = new_level;
	_strip_animator.set_current_animation_idle(_social_level);
}