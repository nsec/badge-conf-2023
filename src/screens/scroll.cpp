/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2023 Jérémie Galarneau <jeremie.galarneau@gmail.com>
 */

#include "display/screen.hpp"
#include "display/scroll.hpp"
#include "display/utils.hpp"
#include "globals.hpp"

namespace nd = nsec::display;
namespace ndu = nsec::display::utils;
namespace nb = nsec::button;
namespace ns = nsec::scheduling;

namespace {
const char default_property[] PROGMEM = "Unset content";
const char repeat_separator[] PROGMEM = "|";
constexpr uint8_t repeat_separator_padding = 4 * nsec::config::display::scroll_font_size;

inline char to_char(const char *src)
{
	return *src;
}

inline char to_char(const __FlashStringHelper *src)
{
	return pgm_read_byte(reinterpret_cast<PGM_P>(src));
}

const __FlashStringHelper *as_flash_string(const char *str)
{
	return static_cast<const __FlashStringHelper *>(static_cast<const void *>(str));
}

template <typename StringType>
unsigned int property_renderable_character_count(const StringType *property)
{
	unsigned int length = 0;
	auto current_ptr = reinterpret_cast<const char *>(property);

	while (to_char(reinterpret_cast<const StringType *>(current_ptr++))) {
		length++;
	}

	return length;
}

unsigned int window_offset_from_frame_time(nsec::scheduling::absolute_time_ms time,
					   uint8_t character_width,
					   uint8_t character_count)
{
	// Compute x offset of the viewport according to the time
	const auto total_string_width =
		(character_width * (character_count + sizeof(repeat_separator) - 1)) +
		(2 * repeat_separator_padding);

	auto window_offset = nsec::config::display::scroll_pixels_per_second * (time / 1000);
	window_offset += ((nsec::config::display::scroll_pixels_per_second) * (time % 1000)) / 1000;
	window_offset %= total_string_width;

	return window_offset;
}
} // namespace

nd::scroll_screen::scroll_screen() noexcept : screen()
{
	_cleared_on_every_frame = false;
	set_property(as_flash_string(default_property));
}

void nd::scroll_screen::button_event(nb::id id, nb::event event) noexcept
{
	if (event == nb::event::UP) {
		// Not interested in any button release events; it allows us
		// to assume the action is a button press or repeat.
		return;
	}

	// Release focus to the previous screen.
	if (id == nb::id::CANCEL) {
		_release_focus();
	}
}

char nd::scroll_screen::_property_character_at_offset(uint8_t offset) const noexcept
{
	return _property.is_value_in_ram ?
		to_char(_property.ram_value + offset) :
		to_char(as_flash_string(reinterpret_cast<const char *>(_property.flash_value) +
					offset));
}

void nd::scroll_screen::_render_current_property_character(Adafruit_SSD1306& canvas) const noexcept
{
	canvas.drawChar(canvas.getCursorX(),
			canvas.getCursorY(),
			_property_character_at_offset(_current_character_offset),
			SSD1306_WHITE,
			SSD1306_BLACK,
			nsec::config::display::scroll_font_size);
	canvas.setCursor(canvas.getCursorX() + _scroll_character_width, canvas.getCursorY());
}

void nd::scroll_screen::_render(scheduling::absolute_time_ms current_time_ms,
				Adafruit_SSD1306& canvas) noexcept
{
	if (!_layout_initialized) {
		_initialize_layout(canvas);
	}

	switch (_render_state()) {
	case render_state::FRAME_SETUP:
	{
		// Compute x offset of the viewport according to the time.
		const unsigned int window_offset =
			window_offset_from_frame_time(current_time_ms,
						      _scroll_character_width,
						      _property.renderable_character_count);

		canvas.setTextSize(nsec::config::display::scroll_font_size);
		canvas.setCursor(-static_cast<int16_t>(window_offset), _scroll_character_y_offset);
		canvas.setTextWrap(false);
		canvas.clearDisplay();
		_render_state(render_state::FIRST_VISIBLE_CHARACTER_RENDERING);
		break;
	}
	case render_state::FIRST_VISIBLE_CHARACTER_RENDERING:
	{
		// "Render" characters until we step into the visible area.
		while (canvas.getCursorX() < 0) {
			_render_current_property_character(canvas);
			_current_character_offset++;

			if (_current_character_offset == _property.renderable_character_count) {
				break;
			}
		}

		_render_state(_current_character_offset < _property.renderable_character_count ?
				      render_state::VISIBLE_CHARACTER_CHUNK_RENDERING :
				      render_state::SEPARATOR_RENDERING);
		break;
	}
	case render_state::VISIBLE_CHARACTER_CHUNK_RENDERING:
		_render_current_property_character(canvas);
		_current_character_offset++;
		if (canvas.getCursorX() < width()) {
			_render_state(_current_character_offset <
						      _property.renderable_character_count ?
					      render_state::VISIBLE_CHARACTER_CHUNK_RENDERING :
					      render_state::SEPARATOR_RENDERING);
		} else {
			// Frame completed.
			_render_state(render_state::FRAME_SETUP);
			canvas.display();
		}

		break;
	case render_state::SEPARATOR_RENDERING:
		canvas.setCursor(canvas.getCursorX() + repeat_separator_padding,
				 canvas.getCursorY());
		canvas.print(as_flash_string(repeat_separator));
		canvas.setCursor(canvas.getCursorX() + repeat_separator_padding,
				 canvas.getCursorY());

		if (canvas.getCursorX() < width()) {
			_render_state(render_state::VISIBLE_CHARACTER_CHUNK_RENDERING);
		} else {
			// Frame completed.
			_render_state(render_state::FRAME_SETUP);
			canvas.display();
		}

		break;
	}

	damage();
}

void nd::scroll_screen::set_property(const __FlashStringHelper *property) noexcept
{
	_property.flash_value = property;
	_property.is_value_in_ram = false;
	_property.renderable_character_count = property_renderable_character_count(property);
}

void nd::scroll_screen::set_property(const char *property) noexcept
{
	_property.ram_value = property;
	_property.is_value_in_ram = true;
	_property.renderable_character_count = property_renderable_character_count(property);
}

void nd::scroll_screen::focused() noexcept
{
	_render_state(render_state::FRAME_SETUP);
	screen::focused();
}

void nd::scroll_screen::_initialize_layout(Adafruit_SSD1306& canvas) noexcept
{
	int x, y;
	unsigned int text_width, text_height;

	canvas.setTextSize(nsec::config::display::scroll_font_size);
	canvas.getTextBounds("a", 0, 0, &x, &y, &text_width, &text_height);
	_scroll_character_width = text_width;
	_scroll_character_y_offset = (canvas.height() - text_height) / 2;
	_layout_initialized = true;
}
