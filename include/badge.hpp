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
#include "display/menu.hpp"
#include "led/strip_animator.hpp"

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

	// Setup hardware.
	void setup();

private:
	// Handle new button event
	void on_button_event(button::id button, button::event event) noexcept;
	void set_social_level(uint8_t new_level);
	void relase_focus_current_screen() noexcept;
	void set_focused_screen(display::screen& focused_screen) noexcept;

	uint8_t _social_level;

	button::watcher _button_watcher;

	// screens
	display::idle_screen _idle_screen;
	display::menu_screen _menu_screen;
	display::screen *_focused_screen;

	// displays
	led::strip_animator _strip_animator;
	display::renderer _renderer;
};
} // namespace nsec::runtime

#endif // NSEC_RUNTIME_BADGE_HPP
