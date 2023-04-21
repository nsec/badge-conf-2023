/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2023 Jérémie Galarneau <jeremie.galarneau@gmail.com>
 */

#include "Adafruit_SSD1306/Adafruit_SSD1306.h"
#include "config.hpp"
#include "display/menu/menu.hpp"
#include "display/utils.hpp"

namespace nd = nsec::display;
namespace ndu = nsec::display::utils;
namespace nb = nsec::button;

nd::menu_screen::menu_screen() noexcept : screen()
{
}

void nd::menu_screen::button_event(nb::id id, nb::event event) noexcept
{
	if (event == nb::event::UP) {
		// Not interested in any button release events; it allows us
		// to assume the action is a button press or repeat.
		return;
	}

	// Release focus to the previous screen.
	if (id == nb::id::CANCEL || id == nb::id::LEFT) {
		_release_focus();
		return;
	}

	if (id == nb::id::OK || id == nb::id::RIGHT) {
		(*_choices)[_active_choice_idx]();
	}

	// Up/down movement.
	if (id == nb::id::UP || id == nb::id::DOWN) {
		const auto choice_direction = id == nb::id::UP ? -1 : 1;
		const auto choice_max_index = static_cast<int>(_choices->count() - 1);
		const int new_choice =
			constrain(static_cast<int>(_active_choice_idx) + choice_direction,
				  0,
				  choice_max_index);

		_active_choice_idx = static_cast<decltype(_active_choice_idx)>(new_choice);
		if (_is_choice_offscreen(_active_choice_idx)) {
			_first_drawn_choice_index += choice_direction;
		}

		// Queue a redraw on next rendering tick.
		damage();
	}
}

void nd::menu_screen::_initialize_layout_constraints(Adafruit_SSD1306& canvas) noexcept
{
	int x, y;
	unsigned int text_width, text_height;

	canvas.setTextSize(config::display::menu_font_size);
	canvas.getTextBounds("a", 0, 0, &x, &y, &text_width, &text_height);

	_layout_constraints.glyph_size.height = text_height;
	_layout_constraints.glyph_size.width = text_width;
	_layout_constraints._lines_per_screen = canvas.height() / text_height;
	_layout_constraints._chars_per_screen = canvas.width() / text_width;
	_layout_constraints_initialized = true;
}

bool nd::menu_screen::_is_choice_offscreen(unsigned int choice) const noexcept
{
	return choice < _first_drawn_choice_index ||
		choice >= static_cast<unsigned int>(_first_drawn_choice_index +
						    _layout_constraints._lines_per_screen);
}

void nd::menu_screen::_render(scheduling::absolute_time_ms current_time_ms,
			      Adafruit_SSD1306& canvas) noexcept
{
	if (!_layout_constraints_initialized) {
		_initialize_layout_constraints(canvas);
	}

	canvas.setCursor(0, 0);
	canvas.setTextSize(nsec::config::display::menu_font_size);

	const auto choices_to_draw_count =
		min(_choices->count(), _layout_constraints._lines_per_screen);
	const auto draw_up_indicator = _first_drawn_choice_index != 0;
	const auto draw_down_indicator = _choices->count() >
		static_cast<unsigned int>(_first_drawn_choice_index +
					  _layout_constraints._lines_per_screen);
	// Reserve a character if we have to draw the scrollbar
	const auto max_line_length = (draw_up_indicator || draw_down_indicator) ?
		_layout_constraints._chars_per_screen - 1 :
		_layout_constraints._chars_per_screen;

	int y_position = 0;
	for (auto line_drawn = 0U; line_drawn < choices_to_draw_count; ++line_drawn) {
		const auto choice_being_drawn_idx = _first_drawn_choice_index + line_drawn;
		canvas.setCursor(0, y_position);

		// Highlight current choice by inverting colors.
		if (choice_being_drawn_idx == _active_choice_idx) {
			canvas.fillRect(0,
					y_position,
					canvas.width(),
					_layout_constraints.glyph_size.height,
					SSD1306_WHITE);
			canvas.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
		} else {
			canvas.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
		}

		ndu::draw_string(canvas, (*_choices)[choice_being_drawn_idx].name, max_line_length);
		y_position += _layout_constraints.glyph_size.height;
	}

	const auto last_char_x_position =
		_layout_constraints.glyph_size.width * (_layout_constraints._chars_per_screen - 1);
	if (draw_down_indicator) {
		// Invert color of down indicator if the active choice is the last one on the
		// screen.
		const auto indicator_color = (_active_choice_idx - _first_drawn_choice_index) ==
				(_layout_constraints._lines_per_screen - 1) ?
			SSD1306_BLACK :
			SSD1306_WHITE;

		// If there are choices remaining, the cursor is already in the right place on the
		// Y-axis (last line).
		ndu::draw_arrow_glyph(canvas,
				      last_char_x_position,
				      canvas.getCursorY(),
				      _layout_constraints.glyph_size.width,
				      _layout_constraints.glyph_size.height,
				      indicator_color,
				      ndu::arrow_glyph_direction::DOWN);
	}

	if (draw_up_indicator) {
		// Invert color of up indicator if the active choice is the first one on the screen.
		const auto indicator_color = (_active_choice_idx - _first_drawn_choice_index) == 0 ?
			SSD1306_BLACK :
			SSD1306_WHITE;

		ndu::draw_arrow_glyph(canvas,
				      last_char_x_position,
				      0,
				      _layout_constraints.glyph_size.width,
				      _layout_constraints.glyph_size.height,
				      indicator_color,
				      ndu::arrow_glyph_direction::UP);
	}
}