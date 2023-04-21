/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2023 Jérémie Galarneau <jeremie.galarneau@gmail.com>
 */

#include "display/splash.hpp"
#include "display/utils.hpp"
#include "logo.hpp"

namespace nd = nsec::display;
namespace nb = nsec::button;

nd::splash_screen::splash_screen(
	const screen::release_focus_notifier& release_focus_notifier) noexcept :
	screen(release_focus_notifier)
{
}

void nd::splash_screen::button_event(nb::id id, nb::event event) noexcept
{
	if (id == nb::id::CANCEL && event != nb::event::UP) {
		_release_focus();
	}
}

void nd::splash_screen::_render(scheduling::absolute_time_ms current_time_ms,
				Adafruit_SSD1306& canvas) noexcept
{
	canvas.drawBitmap(0, 0, nsec_logo_data, canvas.width(), canvas.height(), 1);
}