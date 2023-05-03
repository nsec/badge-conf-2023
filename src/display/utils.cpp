/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2023 Jérémie Galarneau <jeremie.galarneau@gmail.com>
 */

#include <Arduino.h>
#include <display/utils.hpp>

namespace ndu = nsec::display::utils;

namespace {
inline char to_char(const char *src)
{
	return *src;
}

inline char to_char(const __FlashStringHelper *src)
{
	return pgm_read_byte(reinterpret_cast<PGM_P>(src));
}

// Assumes the cursor position is already set.
// Hand-rolled since the canvas Print interface does not allow us to bound the printed length of
// a string stored in program memory.
template <typename StringType>
void _draw_string(Adafruit_SSD1306& canvas,
		  const StringType *str,
		  unsigned int max_char_count,
		  bool draw_ellipsis_if_too_long) noexcept
{
	auto current_ptr = reinterpret_cast<const char *>(str);
	auto limit_ptr = reinterpret_cast<const char *>(str) + max_char_count;

	// First pass to determine if an ellipsis is needed
	bool draw_elipsis = draw_ellipsis_if_too_long;
	if (draw_ellipsis_if_too_long) {
		while (current_ptr != limit_ptr) {
			if (to_char(reinterpret_cast<const StringType *>(current_ptr)) == '\0') {
				draw_elipsis = false;
				break;
			}

			current_ptr++;
		}
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
} // anonymous namespace

void ndu::draw_string(Adafruit_SSD1306& canvas,
		      const char *str,
		      unsigned int max_char_count,
		      bool draw_ellipsis_if_too_long) noexcept
{
	_draw_string(canvas, str, max_char_count, draw_ellipsis_if_too_long);
}
void ndu::draw_string(Adafruit_SSD1306& canvas,
		      const __FlashStringHelper *str,
		      unsigned int max_char_count,
		      bool draw_ellipsis_if_too_long) noexcept
{
	_draw_string(canvas, str, max_char_count, draw_ellipsis_if_too_long);
}

void ndu::draw_arrow_glyph(Adafruit_SSD1306& canvas,
			   unsigned int glyph_x,
			   unsigned int glyph_y,
			   unsigned int glyph_width,
			   unsigned int glyph_heigth,
			   unsigned int glyph_color,
			   ndu::arrow_glyph_direction direction)
{
	const auto is_direction_vertical = direction == arrow_glyph_direction::UP ||
		direction == arrow_glyph_direction::DOWN;

	if (is_direction_vertical) {
		if (glyph_width & 1) {
			glyph_x = glyph_x + 1;
			glyph_width--;
		}
	} else {
		if (glyph_heigth & 1) {
			glyph_y = glyph_y + 1;
			glyph_heigth--;
		}
	}

	int line_draw_direction;
	unsigned int first_segment_drawn;
	if (direction == arrow_glyph_direction::UP || direction == arrow_glyph_direction::LEFT) {
		line_draw_direction = 1;
		first_segment_drawn = direction == arrow_glyph_direction::UP ? glyph_y : glyph_x;
	} else {
		line_draw_direction = -1;
		first_segment_drawn = direction == arrow_glyph_direction::DOWN ?
			(glyph_y + glyph_heigth - 1) :
			(glyph_x + glyph_width - 1);
	}

	unsigned int current_segment_length = 1;
	auto segment_being_drawn = first_segment_drawn;
	auto current_segment_triangle_start = is_direction_vertical ?
		(glyph_x + (glyph_width / 2)) :
		(glyph_y + (glyph_heigth / 2));

	for (auto segments_drawn = 0U;
	     segments_drawn < (is_direction_vertical ? glyph_heigth : glyph_width);
	     segments_drawn++) {
		const auto segment_length_limit = is_direction_vertical ? glyph_width :
									  glyph_heigth;
		if (current_segment_length > segment_length_limit) {
			break;
		}

		if (is_direction_vertical) {
			canvas.writeFastHLine(current_segment_triangle_start,
					      segment_being_drawn,
					      current_segment_length,
					      glyph_color);
		} else {
			canvas.writeFastVLine(segment_being_drawn,
					      current_segment_triangle_start,
					      current_segment_length,
					      glyph_color);
		}

		// Triangle slope is fixed to 1 pixel per line (on each side).
		current_segment_triangle_start--;
		current_segment_length += 2;
		segment_being_drawn += line_draw_direction;
	}
}