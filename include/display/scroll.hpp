/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2023 Jérémie Galarneau <jeremie.galarneau@gmail.com>
 */

#ifndef NSEC_DISPLAY_SCROLL_SCREEN_HPP
#define NSEC_DISPLAY_SCROLL_SCREEN_HPP

#include "config.hpp"
#include "display/screen.hpp"
#include "scheduler.hpp"

namespace nsec::display {

class scroll_screen : public screen {
public:
	explicit scroll_screen() noexcept;

	/* Deactivate copy and assignment. */
	scroll_screen(const scroll_screen&) = delete;
	scroll_screen(scroll_screen&&) = delete;
	scroll_screen& operator=(const scroll_screen&) = delete;
	scroll_screen& operator=(scroll_screen&&) = delete;
	~scroll_screen() override = default;

	void button_event(button::id id, button::event event) noexcept override;
	void _render(scheduling::absolute_time_ms current_time_ms,
		     Adafruit_SSD1306& canvas) noexcept override;

	void set_property(const __FlashStringHelper *property) noexcept;
	void set_property(const char *property) noexcept;

	void focused() noexcept override;

private:
	void _initialize_layout(Adafruit_SSD1306& canvas) noexcept;

	struct {
		union {
			const char *ram_value;
			const __FlashStringHelper *flash_value;
		};
		bool is_value_in_ram : 1;
		uint8_t char_count : 7;
	} _property;

	bool _layout_initialized : 1;
	uint16_t _scroll_character_width : 5;
	unsigned int _scroll_character_y_offset : 5;
};
} // namespace nsec::display

#endif // NSEC_DISPLAY_SCROLL_SCREEN_HPP
