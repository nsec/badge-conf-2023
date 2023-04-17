/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2023 Jérémie Galarneau <jeremie.galarneau@gmail.com>
 */

#ifndef NSEC_DISPLAY_UTILS_HPP
#define NSEC_DISPLAY_UTILS_HPP

#include "Adafruit_SSD1306/Adafruit_SSD1306.h"

namespace nsec::display::utils {

/*
 * Assumes the cursor position is already set.
 * Hand-rolled since the canvas Print interface does not allow us to bound the
 * printed length of a string stored in program memory.
 */
void draw_string(Adafruit_SSD1306& canvas,
		 const char *str,
		 unsigned int max_char_count,
		 bool draw_ellipsis_if_too_long = true) noexcept;
void draw_string(Adafruit_SSD1306& canvas,
		 const __FlashStringHelper *str,
		 unsigned int max_char_count,
		 bool draw_ellipsis_if_too_long = true) noexcept;


enum class arrow_glyph_direction { UP, DOWN, LEFT, RIGHT };

/*
 * Position is in top-left coordinates and indicated the position of the glyph
 * this indicator replaces.
 */
void draw_arrow_glyph(Adafruit_SSD1306& canvas,
		      unsigned int glyph_x,
		      unsigned int glyph_y,
		      unsigned int glyph_width,
		      unsigned int glyph_heigth,
		      unsigned int glyph_color,
		      arrow_glyph_direction direction);

} // namespace nsec::display

#endif // NSEC_DISPLAY_UTILS_SCREEN_HPP
