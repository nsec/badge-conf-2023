/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2023 Jérémie Galarneau <jeremie.galarneau@gmail.com>
 */

#ifndef NSEC_RUNTIME_BADGE_HPP
#define NSEC_RUNTIME_BADGE_HPP

#include "led/strip_animator.hpp"
#include "button/watcher.hpp"

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

	uint8_t _social_level;
	led::strip_animator _strip_animator;
	button::watcher _button_watcher;
};
} // namespace nsec::runtime

#endif // NSEC_RUNTIME_BADGE_HPP
