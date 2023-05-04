/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2023 Jérémie Galarneau <jeremie.galarneau@gmail.com>
 */

#ifndef NSEC_DISPLAY_SCREEN_HPP
#define NSEC_DISPLAY_SCREEN_HPP

#include "../board.hpp"
#include "../button/watcher.hpp"
#include "Adafruit_SSD1306/Adafruit_SSD1306.h"
#include "callback.hpp"
#include "scheduler.hpp"

namespace nsec::display {

using pixel_dimension = uint8_t;

class screen {
public:
	explicit screen() noexcept;

	// Deactivate copy and assignment.
	screen(const screen&) = delete;
	screen(screen&&) = delete;
	screen& operator=(const screen&) = delete;
	screen& operator=(screen&&) = delete;
	~screen() = default;

	// Button event to be handled by screen
	virtual void button_event(button::id id, button::event event) noexcept = 0;

	// Focus gained by screen
	virtual void focused()
	{
		damage();
	}

	bool is_damaged() const noexcept
	{
		return _is_damaged;
	}

	// Render the screen
	void render(scheduling::absolute_time_ms current_time_ms, Adafruit_SSD1306& canvas)
	{
		_is_damaged = false;
		_render(current_time_ms, canvas);
	}

	bool cleared_on_every_frame() const noexcept
	{
		return _cleared_on_every_frame;
	}

protected:
	// Rendering method implemented by derived classes
	virtual void _render(scheduling::absolute_time_ms current_time_ms,
			     Adafruit_SSD1306& canvas) noexcept = 0;

	constexpr pixel_dimension height() const noexcept
	{
		return SCREEN_HEIGHT;
	}
	constexpr pixel_dimension width() const noexcept
	{
		return SCREEN_WIDTH;
	}

	// TODO bubble up the info to the renderer so it can un-schedule itself
	// until the focused screen is damaged.
	void damage()
	{
		_is_damaged = true;
	}

	void _release_focus() noexcept;

	// A screen is only redrawn if it is damaged
	bool _is_damaged : 1;
	bool _cleared_on_every_frame : 1;
};
} // namespace nsec::display

#endif // NSEC_DISPLAY_SCREEN_HPP
