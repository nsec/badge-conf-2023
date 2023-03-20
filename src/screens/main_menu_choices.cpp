#
/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2023 Jérémie Galarneau <jeremie.galarneau@gmail.com>
 */

#include "display/menu/main_menu_choices.hpp"

#include <Arduino.h>

namespace nd = nsec::display;

namespace {
const char user_name_entry_option_name[] PROGMEM = "Edit name";
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
} // anonymous namespace

nd::main_menu_choices::main_menu_choices(
	const choices::choice::menu_choice_action& set_name_action) noexcept :
	_set_name_action{ set_name_action },
	_choices{
		nd::menu_screen::choices::choice(
			as_flash_string(user_name_entry_option_name),
			nd::menu_screen::choices::choice::menu_choice_action(
				[](void *data) {
					reinterpret_cast<nd::main_menu_choices *>(data)
						->_set_name_action();
				},
				this)),
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
	}
{
}

unsigned int nd::main_menu_choices::count() const noexcept
{
	return sizeof(_choices) / sizeof(_choices[0]);
}

const nd::menu_screen::choices::choice&
nd::main_menu_choices::operator[](unsigned int index) const noexcept
{
	return _choices[index];
}
