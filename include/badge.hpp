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

private:
	// Handle new button event
	void on_button_event(button::id button, button::event event) noexcept;
	void set_social_level(uint8_t new_level) noexcept;

	void set_focused_screen(display::screen& focused_screen) noexcept;

	// Handle network events
	void on_disconnection() noexcept;
	void on_pairing_begin() noexcept;
	void on_pairing_end(nsec::communication::peer_id_t our_peer_id,
			    uint8_t peer_count) noexcept;
	nsec::communication::network_handler::application_message_action
	on_message_received(communication::message::type message_type,
			    const uint8_t *message) noexcept;

	uint8_t _social_level : 7;
	uint8_t _button_had_non_repeat_event_since_screen_focus_change;
	char _user_name[nsec::config::user::name_max_length];

	button::watcher _button_watcher;

	// screens
	display::idle_screen _idle_screen;
	display::menu_screen _menu_screen;
	display::string_property_editor_screen _string_property_edit_screen;
	display::splash_screen _splash_screen;
	display::scroll_screen _scroll_screen;
	display::screen *_focused_screen;

	// displays
	led::strip_animator _strip_animator;
	display::renderer _renderer;

	// network
	communication::network_handler _network_handler;

	// menu choices
	display::main_menu_choices _main_menu_choices;
};
} // namespace nsec::runtime

#endif // NSEC_RUNTIME_BADGE_HPP
