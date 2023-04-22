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
unsigned int property_displayable_length(const StringType *property)
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

void nd::scroll_screen::_render(scheduling::absolute_time_ms current_time_ms,
				Adafruit_SSD1306& canvas) noexcept
{
	if (!_layout_initialized) {
		_initialize_layout(canvas);
	}

	// Compute x offset of the viewport according to the time
	const unsigned int window_offset = window_offset_from_frame_time(
		current_time_ms, _scroll_character_width, _property.char_count);

	canvas.setTextSize(nsec::config::display::scroll_font_size);
	canvas.setCursor(-static_cast<int16_t>(window_offset), _scroll_character_y_offset);
	canvas.setTextWrap(false);

	bool first_pass = true;
	while (canvas.getCursorX() < canvas.width()) {
		if (!first_pass) {
			canvas.setCursor(canvas.getCursorX() + repeat_separator_padding,
					 canvas.getCursorY());
			canvas.print(as_flash_string(repeat_separator));
			canvas.setCursor(canvas.getCursorX() + repeat_separator_padding,
					 canvas.getCursorY());
		}

		if (_property.is_value_in_ram) {
			canvas.print(_property.ram_value);
		} else {
			canvas.print(_property.flash_value);
		}

		first_pass = false;
	}

	damage();
}

void nd::scroll_screen::set_property(const __FlashStringHelper *property) noexcept
{
	_property.flash_value = property;
	_property.is_value_in_ram = false;
	_property.char_count = property_displayable_length(property);
}

void nd::scroll_screen::set_property(const char *property) noexcept
{
	_property.ram_value = property;
	_property.is_value_in_ram = true;
	_property.char_count = property_displayable_length(property);
}

void nd::scroll_screen::focused() noexcept
{
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
