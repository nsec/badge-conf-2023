/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2023 Jérémie Galarneau <jeremie.galarneau@gmail.com>
 */

#include "badge.hpp"
#include "board.hpp"
#include "display/menu/menu.hpp"
#include "globals.hpp"
#include "network/network_messages.hpp"

#include <Arduino.h>

namespace nr = nsec::runtime;
namespace nd = nsec::display;
namespace nc = nsec::communication;
namespace nb = nsec::button;

namespace {
const char set_name_prompt[] PROGMEM = "Enter your name";

const __FlashStringHelper *as_flash_string(const char *str)
{
	return static_cast<const __FlashStringHelper *>(static_cast<const void *>(str));
}
} // anonymous namespace

nr::badge::badge() :
	_user_name{ "Kassandra Lapointe-Chagnon" },
	_button_watcher(
		nb::new_button_event_notifier{ [](nsec::button::id id, nsec::button::event event) {
			nsec::g::the_badge.on_button_event(id, event);
		} }),
	_renderer{ &_focused_screen },
	_network_handler{ { [](void *data) {
				   auto *badge = reinterpret_cast<class badge *>(data);

				   badge->on_disconnection();
			   },
			    this },
			  { [](void *data) {
				   auto *badge = reinterpret_cast<class badge *>(data);

				   badge->on_pairing_begin();
			   },
			    this },
			  { [](void *data, nc::peer_id_t id, uint8_t peer_count) {
				   auto *badge = reinterpret_cast<class badge *>(data);

				   badge->on_pairing_end(id, peer_count);
			   },
			    this },
			  { [](void *data,
			       nsec::communication::message::type message_type,
			       const uint8_t *message) {
				   auto *badge = reinterpret_cast<class badge *>(data);

				   return badge->on_message_received(message_type, message);
			   },
			    this } },
	_main_menu_choices{ { [](void *data) {
				     auto *badge = reinterpret_cast<class badge *>(data);

				     badge->_string_property_edit_screen.set_property(
					     as_flash_string(set_name_prompt),
					     badge->_user_name,
					     sizeof(badge->_user_name));
				     badge->set_focused_screen(badge->_string_property_edit_screen);
			     },
			      this } }
{
	set_focused_screen(_splash_screen);
}

void nr::badge::setup()
{
	// GPIO INIT
	pinMode(LED_DBG, OUTPUT);

	_button_watcher.setup();
	_strip_animator.setup();
	_renderer.setup();
	set_social_level(1);

	_network_handler.setup();

	// Hardware serial init (through USB-C connector).
	Serial.begin(38400);
}

void nr::badge::on_button_event(nsec::button::id button, nsec::button::event event) noexcept
{
	const auto button_mask_position = static_cast<unsigned int>(button);

	/*
	 * After a focus change, don't spam the newly focused screen with
	 * repeat events of the button. We want to let the user the time to react,
	 * take their meaty appendage off the button, and initiate new interactions.
	 */
	if (event != nsec::button::event::DOWN_REPEAT) {
		_button_had_non_repeat_event_since_screen_focus_change |= 1 << button_mask_position;
	}

	const auto filter_out_button_event = event == nsec::button::event::DOWN_REPEAT &&
		!(_button_had_non_repeat_event_since_screen_focus_change &
		  (1 << button_mask_position));

	// Forward the event to the focused screen.
	if (!filter_out_button_event) {
		_focused_screen->button_event(button, event);
	}

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
	_button_had_non_repeat_event_since_screen_focus_change = 0;
}

void nr::badge::relase_focus_current_screen() noexcept
{
	if (_focused_screen == &_menu_screen) {
		_scroll_screen.set_property(_user_name);
		set_focused_screen(_scroll_screen);
	} else {
		if (_focused_screen == &_string_property_edit_screen) {
			_string_property_edit_screen.clean_up_property();
		}

		_menu_screen.set_choices(_main_menu_choices);
		set_focused_screen(_menu_screen);
	}
}

void nr::badge::on_disconnection() noexcept
{
	Serial.println(F("Connection lost"));
}

void nr::badge::on_pairing_begin() noexcept
{
}

void nr::badge::on_pairing_end(nc::peer_id_t our_peer_id, uint8_t peer_count) noexcept
{
	Serial.print(F("Connected to network: peer_id="));
	Serial.print(int(our_peer_id));
	Serial.print(F(", peer_count="));
	Serial.println(int(peer_count));
}

nc::network_handler::application_message_action
nr::badge::on_message_received(communication::message::type message_type,
			       const uint8_t *message) noexcept
{
	return nc::network_handler::application_message_action::SWALLOW;
}
