/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2023 Jérémie Galarneau <jeremie.galarneau@gmail.com>
 */

#include "badge.hpp"
#include "board.hpp"
#include "display/menu.hpp"
#include "globals.hpp"

#include <Arduino.h>

namespace nr = nsec::runtime;
namespace nd = nsec::display;

namespace settings_menu {

const char choice_1_name[] PROGMEM = "First choice item";
const char choice_2_name[] PROGMEM = "Second choice item to test long string handling";
const char choice_3_name[] PROGMEM = "Third choice item";
const char choice_4_name[] PROGMEM = "Fourth choice item";
const char choice_5_name[] PROGMEM = "Fifth choice item";
const char choice_6_name[] PROGMEM = "Sixth choice item";
const char choice_7_name[] PROGMEM = "Seventh choice item";
const char choice_8_name[] PROGMEM = "Eighth choice item";

const __FlashStringHelper *as_flash_string(const char *str)
{
	return static_cast<const __FlashStringHelper *>(static_cast<const void *>(str));
}

class choices : public nd::menu_screen::choices {
public:
	choices() = default;

	unsigned int count() const noexcept override
	{
		return sizeof(_choices) / sizeof(_choices[0]);
	}

	const nd::menu_screen::choices::choice& operator[](unsigned int index) const noexcept override
	{
		return _choices[index];
	}

private:
	nd::menu_screen::choices::choice _choices[8] = {
		nd::menu_screen::choices::choice(
			as_flash_string(choice_1_name),
			nd::menu_screen::choices::choice::menu_choice_action(
				[](void *) { Serial.println(as_flash_string(choice_1_name)); },
				nullptr)),
		nd::menu_screen::choices::choice(
			as_flash_string(choice_2_name),
			nd::menu_screen::choices::choice::menu_choice_action(
				[](void *) { Serial.println(as_flash_string(choice_2_name)); },
				nullptr)),
		nd::menu_screen::choices::choice(
			as_flash_string(choice_3_name),
			nd::menu_screen::choices::choice::menu_choice_action(
				[](void *) { Serial.println(as_flash_string(choice_3_name)); },
				nullptr)),
		nd::menu_screen::choices::choice(
			as_flash_string(choice_4_name),
			nd::menu_screen::choices::choice::menu_choice_action(
				[](void *) { Serial.println(as_flash_string(choice_4_name)); },
				nullptr)),
		nd::menu_screen::choices::choice(
			as_flash_string(choice_5_name),
			nd::menu_screen::choices::choice::menu_choice_action(
				[](void *) { Serial.println(as_flash_string(choice_5_name)); },
				nullptr)),
		nd::menu_screen::choices::choice(
			as_flash_string(choice_6_name),
			nd::menu_screen::choices::choice::menu_choice_action(
				[](void *) { Serial.println(as_flash_string(choice_6_name)); },
				nullptr)),
		nd::menu_screen::choices::choice(
			as_flash_string(choice_7_name),
			nd::menu_screen::choices::choice::menu_choice_action(
				[](void *) { Serial.println(as_flash_string(choice_7_name)); },
				nullptr)),
		nd::menu_screen::choices::choice(
			as_flash_string(choice_8_name),
			nd::menu_screen::choices::choice::menu_choice_action(
				[](void *) { Serial.println(as_flash_string(choice_8_name)); },
				nullptr)),
	};
};

choices the_choices;

} // namespace settings_menu

nr::badge::badge() :
	_button_watcher{ { [](nsec::button::id id, nsec::button::event event, void *data) {
				  auto *badge = reinterpret_cast<class badge *>(data);

				  badge->on_button_event(id, event);
			  },
			   this } },
	_idle_screen{ { [](void *data) {
			       auto *badge = reinterpret_cast<class badge *>(data);

			       badge->relase_focus_current_screen();
		       },
			this } },
	_menu_screen{ { [](void *data) {
			       auto *badge = reinterpret_cast<class badge *>(data);

			       badge->relase_focus_current_screen();
		       },
			this } },
	_renderer{ &_focused_screen }
{
	set_focused_screen(_idle_screen);
}

void nr::badge::setup()
{
	// GPIO INIT
	pinMode(LED_DBG, OUTPUT);

	_button_watcher.setup();
	_strip_animator.setup();
	_renderer.setup();
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
	// Forward the event to the focused screen.
	_focused_screen->button_event(button, event);

	// The rest is temporary to easily simulate level up/down.
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

void nr::badge::set_focused_screen(nd::screen& newly_focused_screen) noexcept
{
	_focused_screen = &newly_focused_screen;
	_focused_screen->focused();
}

void nr::badge::relase_focus_current_screen() noexcept
{
	Serial.println("relase_focus_current_screen");
	if (_focused_screen == &_idle_screen) {
		Serial.println("setting menu screen focus");
		_menu_screen.set_choices(settings_menu::the_choices);
		set_focused_screen(_menu_screen);
	} else if (_focused_screen == &_menu_screen) {
		Serial.println("setting idle screen focus");
		set_focused_screen(_idle_screen);
	}
}