/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2023 Jérémie Galarneau <jeremie.galarneau@gmail.com>
 */

#ifndef NSEC_RUNTIME_BADGE_HPP
#define NSEC_RUNTIME_BADGE_HPP

#include "button/watcher.hpp"
#include "display/idle.hpp"
#include "display/renderer.hpp"
#include "display/screen.hpp"
#include "display/menu/menu.hpp"
#include "display/menu/main_menu_choices.hpp"
#include "display/string_property_editor.hpp"
#include "led/strip_animator.hpp"
#include "config.hpp"

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

private:
	// Handle new button event
	void on_button_event(button::id button, button::event event) noexcept;
	void set_social_level(uint8_t new_level);
	void relase_focus_current_screen() noexcept;
	void set_focused_screen(display::screen& focused_screen) noexcept;

	uint8_t _social_level;
	char _user_name[nsec::config::user::name_max_length];

	button::watcher _button_watcher;
	uint8_t _button_had_non_repeat_event_since_screen_focus_change;

	// screens
	display::idle_screen _idle_screen;
	display::menu_screen _menu_screen;
	display::string_property_editor_screen _string_property_edit_screen;
	display::screen *_focused_screen;

	// displays
	led::strip_animator _strip_animator;
	display::renderer _renderer;

	// menu choices
	display::main_menu_choices _main_menu_choices;
};
} // namespace nsec::runtime

#endif // NSEC_RUNTIME_BADGE_HPP
