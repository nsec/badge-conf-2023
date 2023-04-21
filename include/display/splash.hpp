/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2023 Jérémie Galarneau <jeremie.galarneau@gmail.com>
 */

#ifndef NSEC_DISPLAY_SPLASH_SCREEN_HPP
#define NSEC_DISPLAY_SPLASH_SCREEN_HPP

#include "callback.hpp"
#include "display/screen.hpp"
#include "scheduler.hpp"

namespace nsec::display {

class splash_screen : public screen {
public:
	explicit splash_screen() noexcept;

	/* Deactivate copy and assignment. */
	splash_screen(const splash_screen&) = delete;
	splash_screen(splash_screen&&) = delete;
	splash_screen& operator=(const splash_screen&) = delete;
	splash_screen& operator=(splash_screen&&) = delete;
	~splash_screen() override = default;

	void button_event(button::id id, button::event event) noexcept override;
	void _render(scheduling::absolute_time_ms current_time_ms,
		     Adafruit_SSD1306& canvas) noexcept override;
	void focused() noexcept override;

private:
	class one_shot_timer_task : public nsec::scheduling::task {
	public:
		one_shot_timer_task() = default;
		void run(nsec::scheduling::absolute_time_ms current_time) noexcept override;
	};

	one_shot_timer_task _timer;
};
} // namespace nsec::display

#endif // NSEC_DISPLAY_SPLASH_SCREEN_HPP
