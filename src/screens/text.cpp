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

void nd::text_screen::button_event(nb::id id, nb::event event) noexcept
{
	if (id == nb::id::CANCEL && event != nb::event::UP) {
		_release_focus();
	}
}

void nd::text_screen::_render(scheduling::absolute_time_ms current_time_ms,
			      Adafruit_SSD1306& canvas) noexcept
{
	canvas.setCursor(0, 0);
	canvas.setTextSize(1);
	canvas.setTextWrap(true);
	_print(canvas, current_time_ms);
	// This screen is always "damaged" since it could change on every tick.
	damage();
}

void nd::text_screen::set_printer(const text_printer& printer)
{
	_print = printer;
}