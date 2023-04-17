/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2023 Jérémie Galarneau <jeremie.galarneau@gmail.com>
 */

#include "board.hpp"
#include "display/renderer.hpp"
#include "globals.hpp"

namespace nd = nsec::display;
namespace ns = nsec::scheduling;

nd::renderer::renderer(nd::screen **focused_screen) noexcept :
	periodic_task(nsec::config::display::refresh_period_ms),
	_display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET),
	_focused_screen{ focused_screen }
{
	nsec::g::the_scheduler.schedule_task(*this);
}

void nd::renderer::setup() noexcept
{
	_display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS, true, true, _frameBuffer);
	_display.setTextColor(SSD1306_WHITE);
	_display.clearDisplay();
	_display.setTextSize(1);
	_display.display();
}

void nd::renderer::run(scheduling::absolute_time_ms current_time_ms) noexcept
{
	if (!focused_screen().is_damaged()) {
		return;
	}

	_display.clearDisplay();
	const auto entry = millis();
	focused_screen().render(current_time_ms, _display);
	const auto exit = millis();
	_display.display();

	_last_frame_time_ms = exit - entry;
}