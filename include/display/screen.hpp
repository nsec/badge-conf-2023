/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2023 Jérémie Galarneau <jeremie.galarneau@gmail.com>
 */

#ifndef NSEC_DISPLAY_SCREEN_HPP
#define NSEC_DISPLAY_SCREEN_HPP

#include "../board.hpp"
#include "../button/watcher.hpp"
#include "callback.hpp"
#include "scheduler.hpp"

#include "Adafruit_SSD1306/Adafruit_SSD1306.h"

namespace nsec::display {

using pixel_dimension = uint8_t;

class screen {
public:
	// Callable that will be invoked when a focused screen wishes to relinquish the focus.
	using release_focus_notifier = nsec::callback;
	// Callable that will be invoked when a focused screen is damaged and should be re-rendered.
	using damage_notifier = nsec::callback;

	explicit screen(const release_focus_notifier& release_focus_notifier) noexcept :
		_release_focus{ release_focus_notifier }
	{
	}

	// Deactivate copy and assignment.
	screen(const screen&) = delete;
	screen(screen&&) = delete;
	screen& operator=(const screen&) = delete;
	screen& operator=(screen&&) = delete;
	virtual ~screen() = default;

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

	release_focus_notifier _release_focus;
	// A screen is only redrawn if it is damaged
	bool _is_damaged : 1;
};
} // namespace nsec::display

#endif // NSEC_DISPLAY_SCREEN_HPP
