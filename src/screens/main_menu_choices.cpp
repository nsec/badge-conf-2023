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
const char user_name_entry_option_name[] PROGMEM = "Set name";
const char led_config_entry_option_name[] PROGMEM = "Set RGB animation";
const char badge_id_option_name[] PROGMEM = "Badge information";
const char factory_reset_option_name[] PROGMEM = "Factory reset";

const __FlashStringHelper *as_flash_string(const char *str)
{
	return static_cast<const __FlashStringHelper *>(static_cast<const void *>(str));
}
} // anonymous namespace

nd::main_menu_choices::main_menu_choices(
	const choice_action& set_name_action,
	const choice_action& show_badge_info_action) noexcept :
	_set_name_action{ set_name_action },
	_show_badge_info_action{ show_badge_info_action },
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
			as_flash_string(led_config_entry_option_name),
			nd::menu_screen::choices::choice::menu_choice_action(
				[](void *) {
				},
				this)),
		nd::menu_screen::choices::choice(
			as_flash_string(badge_id_option_name),
			nd::menu_screen::choices::choice::menu_choice_action(
				[](void *data) {
					reinterpret_cast<nd::main_menu_choices *>(data)
						->_show_badge_info_action();
				},
				this)),
		nd::menu_screen::choices::choice(
			as_flash_string(factory_reset_option_name),
			nd::menu_screen::choices::choice::menu_choice_action(
				[](void *) {
				},
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
