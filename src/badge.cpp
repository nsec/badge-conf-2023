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
#include <ArduinoUniqueID.h>

namespace nr = nsec::runtime;
namespace nd = nsec::display;
namespace nc = nsec::communication;
namespace nb = nsec::button;

namespace {
const char set_name_prompt[] PROGMEM = "Enter your name";
const char yes_str[] PROGMEM = "yes";
const char no_str[] PROGMEM = "no";

const __FlashStringHelper *as_flash_string(const char *str)
{
	return static_cast<const __FlashStringHelper *>(static_cast<const void *>(str));
}

void badge_info_printer(void *badge_data,
			Print& print,
			nsec::scheduling::absolute_time_ms current_time_ms)
{
	auto *badge = reinterpret_cast<class nsec::runtime::badge *>(badge_data);

	print.print(F("ID: "));
	for (uint8_t i = 0; i < UniqueIDsize; i++) {
		if (UniqueID[i] < 0x10) {
			print.print(F("0"));
		}

		print.print(UniqueID[i], HEX);
		print.print(F(" "));
	}
	print.println();

	print.print(F("Level: "));
	print.println(int(badge->level()));

	print.print(F("Connected: "));
	print.println(badge->is_connected() ? as_flash_string(yes_str) : as_flash_string(no_str));
}

} // anonymous namespace

nr::badge::badge() :
	_user_name{ "Kassandra Lapointe-Chagnon" },
	_button_watcher(
		[](nsec::button::id id, nsec::button::event event) {
			nsec::g::the_badge.on_button_event(id, event);
		}),
	_renderer{ &_focused_screen },
	_network_handler(),
	_main_menu_choices(
		[]() {
			 auto *badge = &nsec::g::the_badge;

			 badge->_string_property_edit_screen.set_property(
				 as_flash_string(set_name_prompt),
				 badge->_user_name,
				 sizeof(badge->_user_name));
			 badge->set_focused_screen(badge->_string_property_edit_screen);
		 },
		[]() {
			 auto *badge = &nsec::g::the_badge;

			 badge->_text_screen.set_printer(
				 nd::text_screen::text_printer{ badge_info_printer, badge });
			 badge->set_focused_screen(badge->_text_screen);
		 }
	)
{
	_network_app_state(network_app_state::UNCONNECTED);
	_id_exchanger.reset();
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

uint8_t nr::badge::level() const noexcept
{
	return _social_level;
}

bool nr::badge::is_connected() const noexcept
{
	return _network_app_state() != network_app_state::UNCONNECTED;
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
	if (_focused_screen == &_string_property_edit_screen) {
		_string_property_edit_screen.clean_up_property();
	}

	_focused_screen = &newly_focused_screen;
	_focused_screen->focused();
	_button_had_non_repeat_event_since_screen_focus_change = 0;
}

void nr::badge::relase_focus_current_screen() noexcept
{
	if (_network_app_state() == network_app_state::EXCHANGING_IDS ||
	    _network_app_state() == network_app_state::ANIMATE_PAIRING) {
		// Lock the user to the pairing screen.
		return;
	}

	if (_focused_screen == &_menu_screen) {
		_scroll_screen.set_property(_user_name);
		set_focused_screen(_scroll_screen);
		return;
	}

	_menu_screen.set_choices(_main_menu_choices);
	set_focused_screen(_menu_screen);
}

void nr::badge::on_disconnection() noexcept
{
	Serial.println(F("Connection lost"));
	_network_app_state(network_app_state::UNCONNECTED);
	_id_exchanger.reset();
	_pairing_animator.reset();

	_menu_screen.set_choices(_main_menu_choices);
	set_focused_screen(_menu_screen);
	_strip_animator.set_current_animation_idle(_social_level);
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

	_scroll_screen.set_property(F("Pairing"));
	set_focused_screen(_scroll_screen);

	_id_exchanger.reset();
	_network_app_state(network_app_state::EXCHANGING_IDS);
	_id_exchanger.start(*this);
}

nc::network_handler::application_message_action
nr::badge::on_message_received(communication::message::type message_type,
			       const uint8_t *message) noexcept
{
	if (_network_app_state() == network_app_state::EXCHANGING_IDS) {
		_id_exchanger.new_message(*this, message_type, message);
	} else if (_network_app_state() == network_app_state::ANIMATE_PAIRING) {
		_pairing_animator.new_message(*this, message_type, message);
	}

	return nc::network_handler::application_message_action::OK;
}

void nr::badge::on_app_message_sent() noexcept
{
	if (_network_app_state() == network_app_state::EXCHANGING_IDS) {
		_id_exchanger.message_sent(*this);
	}
}

void nr::badge::on_splash_complete() noexcept
{
	if (_focused_screen == &_splash_screen) {
		relase_focus_current_screen();
	}
}

nr::badge::network_app_state nr::badge::_network_app_state() const noexcept
{
	return network_app_state(_current_network_app_state);
}

void nr::badge::_network_app_state(nr::badge::network_app_state new_state) noexcept
{
	_current_network_app_state = uint8_t(new_state);
	if (new_state == network_app_state::ANIMATE_PAIRING) {
		_pairing_animator.start(*this);
	} else if (new_state == network_app_state::IDLE ||
		   new_state == network_app_state::UNCONNECTED) {
		set_focused_screen(_menu_screen);
		_strip_animator.set_current_animation_idle(_social_level);
	}
}

nr::badge::badge_discovered_result nr::badge::on_badge_discovered(const uint8_t *id) noexcept
{
	return badge_discovered_result::NEW;
}

void nr::badge::on_badge_discovery_completed() noexcept
{
	Serial.print(F("Discovery completed: "));
	Serial.print(_id_exchanger.new_badges_discovered());
	Serial.println(F(" new badges"));
	_network_app_state(network_app_state::ANIMATE_PAIRING);
}

void nr::badge::network_id_exchanger::start(nr::badge& badge) noexcept
{
	const auto our_id = badge._network_handler.peer_id();

	if (our_id != 0) {
		// Wait for messages from left neighbors.
		return;
	}

	// Left-most peer initiates the exchange.
	nc::message::announce_badge_id msg = { .peer_id = our_id };
	memcpy(msg.board_unique_id, _UniqueID.id, UniqueIDsize);

	badge._network_handler.enqueue_app_message(nc::peer_relative_position::RIGHT,
						   uint8_t(nc::message::type::ANNOUNCE_BADGE_ID),
						   reinterpret_cast<const uint8_t *>(&msg));
}

void nr::badge::network_id_exchanger::new_message(nr::badge& badge,
						  nc::message::type msg_type,
						  const uint8_t *payload) noexcept
{
	if (msg_type != nc::message::type::ANNOUNCE_BADGE_ID) {
		return;
	}

	const auto *announce_badge_id =
		reinterpret_cast<const nc::message::announce_badge_id *>(payload);

	if (badge.on_badge_discovered(announce_badge_id->board_unique_id) ==
	    badge_discovered_result::NEW) {
		_new_badges_discovered++;
	}

	_message_received_count++;

	const auto our_position = badge._network_handler.position();
	const auto our_peer_id = badge._network_handler.peer_id();
	const auto msg_origin_peer_id = announce_badge_id->peer_id;
	const auto peer_count = badge._network_handler.peer_count();

	switch (our_position) {
	case nc::network_handler::link_position::LEFT_MOST:
		if (_message_received_count == peer_count - 1) {
			// Done!
			badge.on_badge_discovery_completed();
		}

		break;
	case nc::network_handler::link_position::RIGHT_MOST:
		if (_message_received_count == peer_count - 1) {
			nc::message::announce_badge_id msg = {
				.peer_id = badge._network_handler.peer_id()
			};
			memcpy(msg.board_unique_id, _UniqueID.id, UniqueIDsize);

			badge._network_handler.enqueue_app_message(
				nc::peer_relative_position(_direction),
				uint8_t(nc::message::type::ANNOUNCE_BADGE_ID),
				reinterpret_cast<const uint8_t *>(&msg));

			badge.on_badge_discovery_completed();
		}

		break;
	case nc::network_handler::link_position::MIDDLE:
	{
		if (abs(our_peer_id - msg_origin_peer_id) == 1) {
			_send_ours_on_next_send_complete = true;
			_done_after_sending_ours = msg_origin_peer_id > our_peer_id;
			_direction = msg_origin_peer_id < our_peer_id ?
				uint8_t(nc::peer_relative_position::RIGHT) :
				uint8_t(nc::peer_relative_position::LEFT);
		}

		// Forward messages from other badges.
		badge._network_handler.enqueue_app_message(
			msg_origin_peer_id < our_peer_id ? nc::peer_relative_position::RIGHT :
							   nc::peer_relative_position::LEFT,
			uint8_t(msg_type),
			payload);
	}
	default:
		// Unreachable.
		return;
	}
}

void nr::badge::network_id_exchanger::message_sent(nr::badge& badge) noexcept
{
	if (!_send_ours_on_next_send_complete) {
		return;
	}

	nc::message::announce_badge_id msg = { .peer_id = badge._network_handler.peer_id() };
	memcpy(msg.board_unique_id, _UniqueID.id, UniqueIDsize);

	badge._network_handler.enqueue_app_message(nc::peer_relative_position(_direction),
						   uint8_t(nc::message::type::ANNOUNCE_BADGE_ID),
						   reinterpret_cast<const uint8_t *>(&msg));

	_send_ours_on_next_send_complete = false;

	if (_done_after_sending_ours) {
		badge.on_badge_discovery_completed();
	}
}

void nr::badge::network_id_exchanger::reset() noexcept
{
	_new_badges_discovered = 0;
	_message_received_count = 0;
	_send_ours_on_next_send_complete = false;
	_done_after_sending_ours = false;
}

nr::badge::pairing_animator::pairing_animator() :
	_timer({ [](void *data, nsec::scheduling::absolute_time_ms current_time_ms) {
			auto *animator = reinterpret_cast<pairing_animator *>(data);

			animator->tick(current_time_ms);
		},
		 this })
{
	_animation_state(animation_state::DONE);
	nsec::g::the_scheduler.schedule_task(_timer);
}

void nr::badge::pairing_animator::_animation_state(animation_state new_state) noexcept
{
	_current_state = uint8_t(new_state);
	_state_counter = 0;
}

nr::badge::pairing_animator::animation_state
nr::badge::pairing_animator::_animation_state() const noexcept
{
	return animation_state(_current_state);
}

void nr::badge::pairing_animator::start(nr::badge& badge) noexcept
{
	if (badge._network_handler.position() == nc::network_handler::link_position::LEFT_MOST) {
		_animation_state(animation_state::LIGHT_UP_UPPER_BAR);
	} else {
		_animation_state(animation_state::WAIT_MESSAGE_ANIMATION_PART_1);
	}
}

void nr::badge::pairing_animator::reset() noexcept
{
	_animation_state(animation_state::DONE);
}

nr::badge::pairing_animator::animation_task::animation_task(
	const nsec::callback<void, nsec::scheduling::absolute_time_ms>& action) :
	periodic_task(250), _run{ action }
{
}

void nr::badge::pairing_animator::animation_task::run(
	nsec::scheduling::absolute_time_ms current_time_ms) noexcept
{
	_run(current_time_ms);
}

void nr::badge::pairing_animator::tick(nsec::scheduling::absolute_time_ms current_time_ms) noexcept
{
	switch (_animation_state()) {
	case animation_state::DONE:
		break;
	case animation_state::WAIT_MESSAGE_ANIMATION_PART_1:
	case animation_state::WAIT_MESSAGE_ANIMATION_PART_2:
	case animation_state::WAIT_DONE:
		break;
	case animation_state::LIGHT_UP_UPPER_BAR:
		nsec::g::the_badge._strip_animator.set_red_to_green_led_progress_bar(
			_state_counter);
		if (_state_counter < 8) {
			_state_counter++;
		} else if (nsec::g::the_badge._network_handler.position() ==
			   nc::network_handler::link_position::RIGHT_MOST) {
			_animation_state(animation_state::LIGHT_UP_LOWER_BAR);
		} else {
			nsec::g::the_badge._network_handler.enqueue_app_message(
				nc::peer_relative_position::RIGHT,
				uint8_t(nc::message::type::PAIRING_ANIMATION_PART_1_DONE),
				nullptr);
			_animation_state(animation_state::WAIT_MESSAGE_ANIMATION_PART_2);
		}

		break;
	case animation_state::LIGHT_UP_LOWER_BAR:
		nsec::g::the_badge._strip_animator.set_red_to_green_led_progress_bar(
			_state_counter + 8);

		if (_state_counter < 8) {
			_state_counter++;
		} else if (nsec::g::the_badge._network_handler.position() ==
			   nc::network_handler::link_position::LEFT_MOST) {
			nsec::g::the_badge._network_handler.enqueue_app_message(
				nc::peer_relative_position::RIGHT,
				uint8_t(nc::message::type::PAIRING_ANIMATION_DONE),
				nullptr);
			_animation_state(animation_state::DONE);
			nsec::g::the_badge._network_app_state(nr::badge::network_app_state::IDLE);
		} else {
			nsec::g::the_badge._network_handler.enqueue_app_message(
				nc::peer_relative_position::LEFT,
				uint8_t(nc::message::type::PAIRING_ANIMATION_PART_2_DONE),
				nullptr);
			_animation_state(animation_state::WAIT_DONE);
		}

		break;
	}
}

void nr::badge::pairing_animator::new_message(nr::badge& badge,
					      nc::message::type msg_type,
					      const uint8_t *payload) noexcept
{
	switch (msg_type) {
	case nc::message::type::PAIRING_ANIMATION_PART_1_DONE:
		_animation_state(animation_state::LIGHT_UP_UPPER_BAR);
		break;
	case nc::message::type::PAIRING_ANIMATION_PART_2_DONE:
		_animation_state(animation_state::LIGHT_UP_LOWER_BAR);
		break;
	case nc::message::type::PAIRING_ANIMATION_DONE:
		if (nsec::g::the_badge._network_handler.position() !=
		    nc::network_handler::link_position::RIGHT_MOST) {
			nsec::g::the_badge._network_handler.enqueue_app_message(
				nc::peer_relative_position::RIGHT,
				uint8_t(nc::message::type::PAIRING_ANIMATION_DONE),
				nullptr);
		}

		nsec::g::the_badge._network_app_state(nr::badge::network_app_state::IDLE);
		break;
	default:
		break;
	}
}