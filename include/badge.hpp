/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2023 Jérémie Galarneau <jeremie.galarneau@gmail.com>
 */

#ifndef NSEC_RUNTIME_BADGE_HPP
#define NSEC_RUNTIME_BADGE_HPP

#include "button/watcher.hpp"
#include "config.hpp"
#include "display/idle.hpp"
#include "display/menu/main_menu_choices.hpp"
#include "display/menu/menu.hpp"
#include "display/renderer.hpp"
#include "display/screen.hpp"
#include "display/scroll.hpp"
#include "display/splash.hpp"
#include "display/string_property_editor.hpp"
#include "display/text.hpp"
#include "led/strip_animator.hpp"
#include "network/network_handler.hpp"

namespace nsec::runtime {

/*
 * Current state of the badge.
 */
class badge {
public:
	badge();

	/* Deactivate copy and assignment. */
	badge(const badge&) = delete;
	badge(badge&&) = delete;
	badge& operator=(const badge&) = delete;
	badge& operator=(badge&&) = delete;
	~badge() = default;

	// Setup hardware.
	void setup();

	void relase_focus_current_screen() noexcept;
	void on_splash_complete() noexcept;
	uint8_t level() const noexcept;
	bool is_connected() const noexcept;

	void on_disconnection() noexcept;
	void on_pairing_begin() noexcept;
	void on_pairing_end(nsec::communication::peer_id_t our_peer_id,
			    uint8_t peer_count) noexcept;
	nsec::communication::network_handler::application_message_action
	on_message_received(communication::message::type message_type,
			    const uint8_t *message) noexcept;
	void on_app_message_sent() noexcept;

private:
	enum class network_app_state {
		UNCONNECTED,
		EXCHANGING_IDS,
		IDLE,
	};
	class network_id_exchanger {
	public:
		network_id_exchanger() = default;

		network_id_exchanger(const network_id_exchanger&) = delete;
		network_id_exchanger(network_id_exchanger&&) = delete;
		network_id_exchanger& operator=(const network_id_exchanger&) = delete;
		network_id_exchanger& operator=(network_id_exchanger&&) = delete;
		~network_id_exchanger() = default;

		void connected(badge&) noexcept;
		void new_message(badge& badge,
				 nsec::communication::message::type msg_type,
				 const uint8_t *payload) noexcept;
		void message_sent(badge& badge) noexcept;
		void reset() noexcept;
		unsigned int new_badges_discovered() const noexcept
		{
			return uint16_t(_new_badges_discovered);
		}

	private:
		uint8_t _new_badges_discovered : 5;
		uint8_t _message_received_count : 5;
		bool _send_ours_on_next_send_complete : 1;
		uint8_t _direction : 1;
		bool _done_after_sending_ours : 1;
	};

	// Handle new button event
	void on_button_event(button::id button, button::event event) noexcept;
	void set_social_level(uint8_t new_level) noexcept;

	void set_focused_screen(display::screen& focused_screen) noexcept;

	// Handle network events
	enum class badge_discovered_result { NEW, ALREADY_KNOWN };
	badge_discovered_result on_badge_discovered(const uint8_t *id) noexcept;
	void on_badge_discovery_completed() noexcept;
	void on_disconnection() noexcept;
	void on_pairing_begin() noexcept;
	void on_pairing_end(nsec::communication::peer_id_t our_peer_id,
			    uint8_t peer_count) noexcept;
	nsec::communication::network_handler::application_message_action
	on_message_received(communication::message::type message_type,
			    const uint8_t *message) noexcept;
	void on_message_sent() noexcept;

	network_app_state _network_app_state() const noexcept;
	void _network_app_state(network_app_state) noexcept;

	uint8_t _social_level : 7;
	// Storage for network_app_state
	uint8_t _current_network_app_state;
	// Mask to prevent repeats after a screen transition, one bit per button.
	uint8_t _button_had_non_repeat_event_since_screen_focus_change;
	char _user_name[nsec::config::user::name_max_length];

	button::watcher _button_watcher;

	// screens
	display::menu_screen _menu_screen;
	display::string_property_editor_screen _string_property_edit_screen;
	display::splash_screen _splash_screen;
	display::scroll_screen _scroll_screen;
	display::text_screen _text_screen;
	display::screen *_focused_screen;

	// displays
	led::strip_animator _strip_animator;
	display::renderer _renderer;

	// network
	communication::network_handler _network_handler;
	network_id_exchanger _id_exchanger;

	// menu choices
	display::main_menu_choices _main_menu_choices;
};
} // namespace nsec::runtime

#endif // NSEC_RUNTIME_BADGE_HPP
