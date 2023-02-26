/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2023 Jérémie Galarneau <jeremie.galarneau@gmail.com>
 */

#ifndef NSEC_DISPLAY_IDLE_SCREEN_HPP
#define NSEC_DISPLAY_IDLE_SCREEN_HPP

#include "display/screen.hpp"

namespace nsec::display {

class idle_screen : public screen {
public:
	explicit idle_screen(const screen::release_focus_notifier& release_focus_notifier) noexcept;

	/* Deactivate copy and assignment. */
	idle_screen(const idle_screen&) = delete;
	idle_screen(idle_screen&&) = delete;
	idle_screen& operator=(const idle_screen&) = delete;
	idle_screen& operator=(idle_screen&&) = delete;
	~idle_screen() override = default;

	enum class result {
		OK,
		/* Screen wants to hand-off focus. */
		QUIT,
	};

	void button_event(button::id id, button::event event) noexcept override;
	void render(scheduling::absolute_time_ms current_time_ms,
		    Adafruit_SSD1306& canvas) noexcept override;

private:
	void randomize_velocity() noexcept;

	// top-left coords
	int16_t _pos_x:9;
	int16_t _pos_y:6;
	int16_t _text_width:9;
	int16_t _text_height:6;
	// direction of bouncy object
	bool _moving_right : 1;
	bool _moving_down : 1;
	int8_t _velocity_x : 4;
	int8_t _velocity_y : 4;
};
} // namespace nsec::display

#endif // NSEC_DISPLAY_IDLE_SCREEN_HPP
