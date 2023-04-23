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

namespace {
constexpr uint16_t render_time_sampling_period = 300;
};

nd::renderer::renderer(nd::screen **focused_screen) noexcept :
	periodic_task(nsec::config::display::refresh_period_ms),
	_display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET),
	_render_time_sampling_counter{ 0 },
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

	const auto entry = millis();

	if (focused_screen().cleared_on_every_frame()) {
		_display.clearDisplay();
	}

	focused_screen().render(current_time_ms, _display);
	const auto exit = millis();

	if (focused_screen().cleared_on_every_frame()) {
		_display.display();
	}

	if (++_render_time_sampling_counter == render_time_sampling_period) {
		const auto last_frame_duration_ms = exit - entry;

		Serial.print(F("Frame time (ms): "));
		Serial.println(last_frame_duration_ms);
		_render_time_sampling_counter = 0;
	}
}