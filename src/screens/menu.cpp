/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2023 Jérémie Galarneau <jeremie.galarneau@gmail.com>
 */

#include "config.hpp"
#include "display/menu.hpp"

#include <Adafruit_SSD1306.h>

namespace nd = nsec::display;
namespace nb = nsec::button;

namespace {
static inline char to_char(const char *src)
{
	return *src;
}

static inline char to_char(const __FlashStringHelper *src)
{
	return pgm_read_byte(reinterpret_cast<PGM_P>(src));
}

// Assumes the cursor position is already set.
// Hand-rolled since the canvas Print interface does not allow us to bound the printed length of
// a string stored in program memory.
template <typename StringType>
void draw_string(Adafruit_SSD1306& canvas,
		 const StringType *str,
		 unsigned int max_char_count) noexcept
{
	auto current_ptr = reinterpret_cast<const char *>(str);
	auto limit_ptr = reinterpret_cast<const char *>(str) + max_char_count;

	bool draw_elipsis = true;
	while (current_ptr != limit_ptr) {
		if (to_char(reinterpret_cast<const StringType *>(current_ptr)) == '\0') {
			draw_elipsis = false;
			break;
		}

		current_ptr++;
	}

	current_ptr = reinterpret_cast<const char *>(str);
	while (current_ptr != limit_ptr) {
		auto current_char = to_char(reinterpret_cast<const StringType *>(current_ptr));
		if (current_char == '\0') {
			return;
		}

		if ((limit_ptr - current_ptr) <= 3 && draw_elipsis) {
			current_char = '.';
		}

		canvas.write(current_char);
		current_ptr++;
	}
}

enum class arrow_glyph_direction { UP, DOWN };

// Position is in top-left coordinates and indicated the position of the glyph
// this indicator replaces
void draw_arrow_glyph(Adafruit_SSD1306& canvas,
		      unsigned int glyph_x,
		      unsigned int glyph_y,
		      unsigned int glyph_width,
		      unsigned int glyph_heigth,
		      unsigned int glyph_color,
		      arrow_glyph_direction direction)
{
	if (glyph_width & 1) {
		glyph_x = glyph_x + 1;
		glyph_width--;
	}

	int line_draw_direction;
	unsigned int first_line_drawn;
	if (direction == arrow_glyph_direction::UP) {
		line_draw_direction = 1;
		first_line_drawn = glyph_y;
	} else {
		line_draw_direction = -1;
		first_line_drawn = glyph_y + glyph_heigth - 1;
	}

	unsigned int current_line_width = 1;
	auto line_being_drawn = first_line_drawn;
	auto current_line_triangle_start = glyph_x + (glyph_width / 2);

	for (auto lines_drawn = 0U; lines_drawn < glyph_heigth; lines_drawn++) {
		if (current_line_width > glyph_width) {
			break;
		}

		canvas.writeFastHLine(current_line_triangle_start,
				      line_being_drawn,
				      current_line_width,
				      glyph_color);

		// Triangle slope is fixed to 1 pixel per line (on each side).
		current_line_triangle_start--;
		current_line_width += 2;
		line_being_drawn += line_draw_direction;
	}
}
} // namespace

nd::menu_screen::menu_screen(const screen::release_focus_notifier& release_focus_notifier) noexcept
	:
	screen(release_focus_notifier)
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

		draw_string(canvas, (*_choices)[choice_being_drawn_idx].name, max_line_length);
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
		draw_arrow_glyph(canvas,
				 last_char_x_position,
				 canvas.getCursorY(),
				 _layout_constraints.glyph_size.width,
				 _layout_constraints.glyph_size.height,
				 indicator_color,
				 arrow_glyph_direction::DOWN);
	}

	if (draw_up_indicator) {
		// Invert color of up indicator if the active choice is the first one on the screen.
		const auto indicator_color = (_active_choice_idx - _first_drawn_choice_index) == 0 ?
			SSD1306_BLACK :
			SSD1306_WHITE;

		draw_arrow_glyph(canvas,
				 last_char_x_position,
				 0,
				 _layout_constraints.glyph_size.width,
				 _layout_constraints.glyph_size.height,
				 indicator_color,
				 arrow_glyph_direction::UP);
	}
}