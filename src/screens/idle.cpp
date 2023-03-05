/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2023 Jérémie Galarneau <jeremie.galarneau@gmail.com>
 */

#include "display/idle.hpp"

namespace nd = nsec::display;
namespace nb = nsec::button;

constexpr auto text_size = 1;
constexpr auto min_velocity = 1;
constexpr auto max_velocity = 4;
constexpr auto max_text_length = 32;

nd::idle_screen::idle_screen(const screen::release_focus_notifier& release_focus_notifier) noexcept
	:
	screen(release_focus_notifier),
	_pos_x{ 0 },
	_pos_y{ 0 },
	_text_width{ 0 },
	_text_height{ 0 },
	_moving_right{ true },
	_moving_down{ true }
{
	randomize_velocity();
}

void nd::idle_screen::button_event(nb::id id, nb::event event) noexcept
{
	if (id == nb::id::CANCEL && event != nb::event::UP) {
		_release_focus();
	}
}

void nd::idle_screen::randomize_velocity() noexcept
{
	_velocity_x = static_cast<int8_t>(random(min_velocity, max_velocity));
	_velocity_y = static_cast<int8_t>(random(min_velocity, max_velocity));
}

void nd::idle_screen::_render(scheduling::absolute_time_ms current_time_ms,
			     Adafruit_SSD1306& canvas) noexcept
{
	const auto text = F("nsec.io");
	const auto x_increment = _moving_right ? _velocity_x : -_velocity_x;
	const auto y_increment = _moving_down ? _velocity_y : -_velocity_y;

	// Initialize the bounding-box on first frame
	if (_text_width == 0) {
		int16_t x, y;
		uint16_t text_width, text_height;
		char local_text[max_text_length];

		strncpy_P(local_text, (const char *) text, max_text_length);
		canvas.getTextBounds(local_text, 0, 0, &x, &y, &text_width, &text_height);
		_text_width = text_width;
		_text_height = text_height;
	}

	if ((_pos_x + _text_width >= width() && _moving_right) || (_pos_x <= 0 && !_moving_right)) {
		_moving_right = !_moving_right;
		randomize_velocity();
	}

	if ((_pos_y + _text_height >= height() && _moving_down) || (_pos_y <= 0 && !_moving_down)) {
		_moving_down = !_moving_down;
	}

	_pos_x = constrain(static_cast<int16_t>(_pos_x),
			   static_cast<int16_t>(0),
			   static_cast<int16_t>(width() - _text_width));
	_pos_y = constrain(static_cast<int16_t>(_pos_y),
			   static_cast<int16_t>(0),
			   static_cast<int16_t>(height() - _text_height));

	canvas.setCursor(_pos_x, _pos_y);
	canvas.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
	canvas.setTextWrap(false);
	canvas.setTextSize(text_size);
	canvas.print(text);

	_pos_x += x_increment;
	_pos_y += y_increment;

	// This screen is always "damaged" since it changes on every tick.
	damage();
}