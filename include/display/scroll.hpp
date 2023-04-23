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
		uint8_t renderable_character_count : 7;
	} _property;

	enum class render_state {
		FRAME_SETUP = 0,
		FIRST_VISIBLE_CHARACTER_RENDERING = 1,
		VISIBLE_CHARACTER_CHUNK_RENDERING = 2,
		SEPARATOR_RENDERING = 3,
	};

	render_state _render_state() const noexcept
	{
		return static_cast<render_state>(_frame_render_state);
	}

	void _render_state(render_state state) noexcept
	{
		_frame_render_state = static_cast<uint8_t>(state);
		if (state == render_state::FRAME_SETUP ||
		    state == render_state::SEPARATOR_RENDERING) {
			_current_character_offset = 0;
		}
	}

	char _property_character_at_offset(uint8_t offset) const noexcept;
	void _render_current_property_character(Adafruit_SSD1306& canvas) const noexcept;

	bool _layout_initialized : 1;
	unsigned int _scroll_character_width : 5;
	unsigned int _scroll_character_y_offset : 5;
	uint8_t _frame_render_state : 2;
	uint8_t _current_character_offset;
};
} // namespace nsec::display

#endif // NSEC_DISPLAY_SCROLL_SCREEN_HPP
