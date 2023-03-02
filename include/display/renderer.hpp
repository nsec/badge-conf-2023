/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2023 Jérémie Galarneau <jeremie.galarneau@gmail.com>
 */

#ifndef NSEC_DISPLAY_RENDERER_HPP
#define NSEC_DISPLAY_RENDERER_HPP

#include "scheduler.hpp"
#include "screen.hpp"

#include <Adafruit_SSD1306.h>

namespace nsec::display {

class renderer : public scheduling::periodic_task {
public:
	explicit renderer(screen **focused_screen) noexcept;

	/* Deactivate copy and assignment. */
	renderer(const renderer&) = delete;
	renderer(renderer&&) = delete;
	renderer& operator=(const renderer&) = delete;
	renderer& operator=(renderer&&) = delete;
	~renderer() override = default;

	void setup() noexcept;

protected:
	void run(scheduling::absolute_time_ms current_time_ms) noexcept override;

private:
	screen& focused_screen() const noexcept
	{
		return **_focused_screen;
	}

	Adafruit_SSD1306 _display;
	scheduling::relative_time_ms _last_frame_time_ms = 0;

	screen **const _focused_screen;
};
} // namespace nsec::display

#endif // NSEC_DISPLAY_RENDERER_HPP