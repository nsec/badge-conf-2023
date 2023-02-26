/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2023 Jérémie Galarneau <jeremie.galarneau@gmail.com>
 */

#ifndef NSEC_DISPLAY_SCREEN_HPP
#define NSEC_DISPLAY_SCREEN_HPP

#include "../board.hpp"
#include "../button/watcher.hpp"
#include "scheduler.hpp"

#include <Adafruit_SSD1306.h>

namespace nsec::display {

using pixel_dimension = uint8_t;

class screen {
public:
	/*
	 * Callable that will be invoked when a focused screen wishes to relinquish the focus.
	 */
	class release_focus_notifier {
	public:
		using release_focus_cb = void (*)(void *data);

		release_focus_notifier(release_focus_cb function, void *data) noexcept :
			_function{ function }, _user_data{ data }
		{
		}

		void operator()() const
		{
			_function(_user_data);
		}

	private:
		release_focus_cb _function;
		void *_user_data;
	};

	explicit screen(const release_focus_notifier& release_focus_notifier) noexcept :
		_release_focus{ release_focus_notifier }
	{
	}

	/* Deactivate copy and assignment. */
	screen(const screen&) = delete;
	screen(screen&&) = delete;
	screen& operator=(const screen&) = delete;
	screen& operator=(screen&&) = delete;
	virtual ~screen() = default;

	virtual void button_event(button::id id, button::event event) noexcept = 0;
	virtual void render(scheduling::absolute_time_ms current_time_ms,
			    Adafruit_SSD1306& canvas) noexcept = 0;

protected:
	constexpr pixel_dimension height() const noexcept
	{
		return SCREEN_HEIGHT;
	}
	constexpr pixel_dimension width() const noexcept
	{
		return SCREEN_WIDTH;
	}

	release_focus_notifier _release_focus;
};
} // namespace nsec::display

#endif // NSEC_DISPLAY_SCREEN_HPP
