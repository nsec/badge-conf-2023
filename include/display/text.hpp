/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2023 Jérémie Galarneau <jeremie.galarneau@gmail.com>
 */

#ifndef NSEC_DISPLAY_TEXT_SCREEN_HPP
#define NSEC_DISPLAY_TEXT_SCREEN_HPP

#include "display/screen.hpp"
#include "callback.hpp"

namespace nsec::display {

class text_screen : public screen {
public:
        using text_printer = nsec::callback<void, Print&, nsec::scheduling::absolute_time_ms>;

	text_screen() noexcept;

	/* Deactivate copy and assignment. */
	text_screen(const text_screen&) = delete;
	text_screen(text_screen&&) = delete;
	text_screen& operator=(const text_screen&) = delete;
	text_screen& operator=(text_screen&&) = delete;

        void set_printer(const text_printer& printer);
	void button_event(button::id id, button::event event) noexcept override;
	void _render(scheduling::absolute_time_ms current_time_ms,
		    Adafruit_SSD1306& canvas) noexcept override;
private:
        text_printer _print;
};
} // namespace nsec::display

#endif // NSEC_DISPLAY_TEXT_SCREEN_HPP
