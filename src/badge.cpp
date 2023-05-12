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
#include "unique_id.hpp"

#include <Arduino.h>

namespace nr = nsec::runtime;
namespace nd = nsec::display;
namespace nc = nsec::communication;
namespace nb = nsec::button;
namespace nl = nsec::led;

namespace {
const char set_name_prompt[] PROGMEM = "Enter your name";
const char yes_str[] PROGMEM = "yes";
const char no_str[] PROGMEM = "no";
const char unset_name_scroll[] PROGMEM = "Press X to set your name";

constexpr uint16_t config_version_magic = 0xBAD8;

class formatter : public Print {
public:
	explicit formatter(char *msg) : _ptr{ msg }
	{
	}

	size_t write(uint8_t new_char) override
	{
		*_ptr = new_char;
		_ptr++;
		return 1;
	}

private:
	char *_ptr;
};

const __FlashStringHelper *as_flash_string(const char *str)
{
	return static_cast<const __FlashStringHelper *>(static_cast<const void *>(str));
}

void badge_info_printer(void *badge_data,
			Print& print,
			nsec::scheduling::absolute_time_ms current_time_ms)
{
	const auto *badge = reinterpret_cast<const class nsec::runtime::badge *>(badge_data);

	// Print the last 4 bytes of the serial unique ID
	print.print(F("ID: "));
	for (uint8_t i = UniqueIDsize - 4; i < UniqueIDsize; i++) {
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

void factory_reset_confirmation_printer(void *, Print& print, nsec::scheduling::absolute_time_ms)
{
	print.print(F("Hold Okay to confirm"));
}

} // anonymous namespace

nr::badge::badge() :
	_is_user_name_set{ false },
	_user_name{ "" },
	_button_watcher([](nsec::button::id id, nsec::button::event event) {
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
		},
		[]() {
			auto *badge = &nsec::g::the_badge;

			badge->_text_screen.set_printer(nd::text_screen::text_printer{
				factory_reset_confirmation_printer, badge });
			badge->set_focused_screen(badge->_text_screen);
			badge->_is_expecting_factory_reset = true;
		})
{
	_network_app_state(network_app_state::UNCONNECTED);
	_id_exchanger.reset();
	set_focused_screen(_splash_screen);
	set_social_level(1, false);
}

void nr::badge::load_config()
{
	eeprom_config config;

	EEPROM.get(0, config);
	if (config.version_magic != config_version_magic) {
		return;
	}

	set_social_level(config.social_level, false);
	if (config.is_name_set) {
		memcpy(_user_name, config.name, sizeof(_user_name));
		_is_user_name_set = config.is_name_set;
	}
}

void nr::badge::save_config() const
{
	eeprom_config config;
	config.version_magic = config_version_magic;
	config.favorite_animation = 0;
	config.is_name_set = _is_user_name_set;
	config.social_level = _social_level;

	memcpy(config.name, _user_name, sizeof(config.name));
	EEPROM.put(0, config);
}

void nr::badge::factory_reset()
{
	eeprom_config config;

	// Set an invalid magic.
	config.version_magic = 1234;

	EEPROM.put(0, config);
	_id_buffer.clear();

	void (*so_looooong)(void) = nullptr;
	so_looooong();
}

void nr::badge::setup()
{
	// GPIO INIT
	pinMode(LED_DBG, OUTPUT);

	_button_watcher.setup();
	_strip_animator.setup();
	_renderer.setup();

	_network_handler.setup();

	// Hardware serial init (through USB-C connector).
	Serial.begin(38400);

	load_config();
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

	// Hackish button handling for the generic "text" screen.
	if (_is_expecting_factory_reset && event == nsec::button::event::DOWN_REPEAT &&
	    button == button::id::OK && !filter_out_button_event) {
		factory_reset();
	}

	// The rest is temporary to easily simulate level up/down.
	switch (button) {
	case nsec::button::id::UP:
		set_social_level(_social_level + 1, false);
		break;
	case nsec::button::id::DOWN:
		set_social_level(_social_level - 1, false);
		break;
	default:
		break;
	}
}

void nr::badge::set_social_level(uint8_t new_level, bool save)
{
	new_level = max(1, new_level);
	new_level = min(nsec::config::social::max_level, new_level);

	_social_level = new_level;
	_strip_animator.set_current_animation_idle(_social_level);

	if (save) {
		save_config();
	}
}

void nr::badge::set_focused_screen(nd::screen& newly_focused_screen) noexcept
{
	if (_focused_screen == &_string_property_edit_screen) {
		_string_property_edit_screen.clean_up_property();
		_is_user_name_set = true;
		save_config();
	}

	_focused_screen = &newly_focused_screen;
	_focused_screen->focused();
	_button_had_non_repeat_event_since_screen_focus_change = 0;
	_is_expecting_factory_reset = false;
}

void nr::badge::relase_focus_current_screen() noexcept
{
	if (_network_app_state() == network_app_state::EXCHANGING_IDS ||
	    _network_app_state() == network_app_state::ANIMATE_PAIRING) {
		// Lock the user to the pairing screen.
		return;
	}

	if (_focused_screen == &_menu_screen || _focused_screen == &_splash_screen) {
		if (_is_user_name_set) {
			_scroll_screen.set_property(_user_name);
		} else {
			_scroll_screen.set_property(as_flash_string(unset_name_scroll));
		}

		set_focused_screen(_scroll_screen);
	} else {
		_menu_screen.set_choices(_main_menu_choices);
		set_focused_screen(_menu_screen);
	}
}

void nr::badge::on_disconnection() noexcept
{
	Serial.println(F("Connection lost"));
	_network_app_state(network_app_state::UNCONNECTED);
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

	_network_app_state(network_app_state::ANIMATE_PAIRING);
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
	_id_exchanger.reset();
	_pairing_animator.reset();
	_pairing_completed_animator.reset();

	_current_network_app_state = uint8_t(new_state);
	switch (new_state) {
	case network_app_state::ANIMATE_PAIRING:
	case network_app_state::EXCHANGING_IDS:
		_scroll_screen.set_property(F("Pairing"));
		set_focused_screen(_scroll_screen);

		// Reduce the CPU time of the renderer to reduce network errors.
		_renderer.period_ms(16);
		if (new_state == network_app_state::ANIMATE_PAIRING) {
			_pairing_animator.start(*this);
		} else {
			_id_exchanger.start(*this);
		}

		break;
	case network_app_state::ANIMATE_PAIRING_COMPLETED:
		_pairing_completed_animator.start(*this);
		break;
	case network_app_state::IDLE:
	case network_app_state::UNCONNECTED:
		relase_focus_current_screen();
		_strip_animator.set_current_animation_idle(_social_level);
		_renderer.period_ms(nsec::config::display::refresh_period_ms);
		break;
	}
}

nr::badge::badge_discovered_result nr::badge::on_badge_discovered(const uint8_t *id) noexcept
{
	/*
	 * Since the chips were all sources from the same supplier in one batch,
	 * their IDs are fairly close together. This makes the use of the last 4
	 * bytes as a "unique" ID acceptable in our context.
	 */
	const uint32_t id_low = (uint32_t(id[6]) << 24) | (uint32_t(id[7]) << 16) |
		(uint32_t(id[8]) << 8) | id[9];

	const auto inserted = _id_buffer.insert(id_low);
	return inserted ? badge_discovered_result::NEW : badge_discovered_result::ALREADY_KNOWN;
}

void nr::badge::on_badge_discovery_completed() noexcept
{
	Serial.print(F("Discovery completed: "));
	Serial.print(_id_exchanger.new_badges_discovered());
	Serial.println(F(" new badges"));
	_badges_discovered_last_exchange = _id_exchanger.new_badges_discovered();
	_network_app_state(network_app_state::ANIMATE_PAIRING_COMPLETED);
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

nr::badge::pairing_animator::pairing_animator()
{
	_animation_state(animation_state::DONE);
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

	badge._timer.period_ms(nsec::config::badge::pairing_animation_time_per_led_progress_bar_ms);
	nsec::g::the_badge._strip_animator.set_red_to_green_led_progress_bar(0);
}

void nr::badge::pairing_animator::reset() noexcept
{
	_animation_state(animation_state::DONE);
}

nr::badge::animation_task::animation_task() : periodic_task(250)
{
	nsec::g::the_scheduler.schedule_task(*this);
}

void nr::badge::animation_task::run(nsec::scheduling::absolute_time_ms current_time_ms) noexcept
{
	nsec::g::the_badge.tick(current_time_ms);
}

void nr::badge::pairing_animator::tick(nsec::scheduling::absolute_time_ms current_time_ms) noexcept
{
	switch (_animation_state()) {
	case animation_state::DONE:
		if (_state_counter < 8 &&
		    nsec::g::the_badge._network_handler.position() ==
			    nc::network_handler::link_position::LEFT_MOST) {
			// Left-most badge waits for the neighbor to transition states.
			_state_counter++;
		} else {
			nsec::g::the_badge._network_app_state(
				nr::badge::network_app_state::EXCHANGING_IDS);
		}
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

		_animation_state(animation_state::DONE);
		break;
	default:
		break;
	}
}

void nr::badge::tick(nsec::scheduling::absolute_time_ms current_time_ms) noexcept
{
	switch (_network_app_state()) {
	case network_app_state::ANIMATE_PAIRING:
		_pairing_animator.tick(current_time_ms);
		break;
	case network_app_state::ANIMATE_PAIRING_COMPLETED:
		_pairing_completed_animator.tick(*this, current_time_ms);
		break;
	default:
		break;
	}
}

void nr::badge::pairing_completed_animator::start(nr::badge& badge) noexcept
{
	badge._timer.period_ms(1000);
	badge._strip_animator.set_pairing_completed_animation(
		badge._badges_discovered_last_exchange > 0 ?
			nl::strip_animator::pairing_completed_animation_type::HAPPY_CLOWN_BARF :
			nl::strip_animator::pairing_completed_animation_type::SAD_AND_LONELY);
	memset(current_message, 0, sizeof(current_message));

	// format current msg
	formatter message_formatter(current_message);
	message_formatter.print(badge._badges_discovered_last_exchange);
	message_formatter.print(F(" "));
	message_formatter.print(F("new badge"));
	if (badge._badges_discovered_last_exchange > 1) {
		message_formatter.print(F("s"));
	}

	message_formatter.print(F(" discovered"));
	if (badge._badges_discovered_last_exchange > 0) {
		message_formatter.print(F("!"));
	}

	badge._scroll_screen.set_property(current_message);
}

void nr::badge::pairing_completed_animator::reset() noexcept
{
	_animation_state(animation_state::SHOW_PAIRING_COMPLETE_MESSAGE);
	_state_counter = 0;
}

void nr::badge::pairing_completed_animator::tick(
	nr::badge& badge, nsec::scheduling::absolute_time_ms current_time_ms) noexcept
{
	_state_counter++;
	if (_state_counter == 8) {
		_state_counter = 0;
		if (_animation_state() == animation_state::SHOW_PAIRING_COMPLETE_MESSAGE) {
			_animation_state(animation_state::SHOW_NEW_LEVEL);

			const auto new_level =
				badge._social_level + badge._badges_discovered_last_exchange;

			badge._strip_animator.set_show_level_animation(
				badge._badges_discovered_last_exchange > 0 ?
					nl::strip_animator::pairing_completed_animation_type::
						HAPPY_CLOWN_BARF :
					nl::strip_animator::pairing_completed_animation_type::
						SAD_AND_LONELY,
				new_level);

			memset(current_message, 0, sizeof(current_message));

			formatter message_formatter(current_message);
			message_formatter.print(F("Level "));
			message_formatter.print(int(new_level));
			badge._scroll_screen.set_property(current_message);
		} else {
			badge.apply_score_change(badge._badges_discovered_last_exchange);
			badge._network_app_state(network_app_state::IDLE);
		}
	}
}

void nr::badge::pairing_completed_animator::_animation_state(
	nr::badge::pairing_completed_animator::animation_state new_state) noexcept
{
	_current_state = uint8_t(new_state);
}

nr::badge::pairing_completed_animator::animation_state
nr::badge::pairing_completed_animator::_animation_state() const noexcept
{
	return animation_state(_current_state);
}

void nr::badge::apply_score_change(uint8_t new_badges_discovered_count) noexcept
{
	// Saves to EEPROM
	set_social_level(_social_level + new_badges_discovered_count);
}