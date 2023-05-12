/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2023 Jérémie Galarneau <jeremie.galarneau@gmail.com>
 */

#include "display/text.hpp"
#include "display/utils.hpp"

namespace nd = nsec::display;
namespace nb = nsec::button;

nd::text_screen::text_screen() noexcept :
	screen(), _print([](void *, Print&, nsec::scheduling::absolute_time_ms) {}, nullptr)
{
}

void nd::text_screen::_render(scheduling::absolute_time_ms current_time_ms,
			      Adafruit_SSD1306& canvas) noexcept
{
	// Reset text rendering properties
	canvas.setCursor(0, 0);
	canvas.setTextSize(1);
	canvas.setTextWrap(true);
	canvas.setTextColor(SSD1306_WHITE);
	_print(canvas, current_time_ms);
}

void nd::text_screen::set_printer(const text_printer& printer)
{
	_print = printer;
}