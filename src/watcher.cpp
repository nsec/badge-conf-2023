/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2023 Jérémie Galarneau <jeremie.galarneau@gmail.com>
 */

#include "board.hpp"
#include "button/watcher.hpp"
#include "config.hpp"
#include "globals.hpp"

#include <Arduino.h>

namespace nb = nsec::button;
namespace ns = nsec::scheduling;
namespace ng = nsec::g;

nb::watcher::watcher(nb::new_button_event_notifier new_button_notifier) noexcept :
	ns::periodic_task(nsec::config::button::polling_period_ms),
	_notify_new_event{ new_button_notifier }
{
	ng::the_scheduler.schedule_task(*this);
}

void nb::watcher::setup() noexcept
{
	// Buttons
	pinMode(BTN_RIGHT, INPUT_PULLUP);
	pinMode(BTN_UP, INPUT_PULLUP);
	pinMode(BTN_DOWN, INPUT_PULLUP);
	pinMode(BTN_LEFT, INPUT_PULLUP);
	pinMode(BTN_OK, INPUT_PULLUP);
	pinMode(BTN_CANCEL, INPUT_PULLUP);
}

void nb::watcher::run([[maybe_unused]] ns::absolute_time_ms current_time_ms) noexcept
{
	const unsigned int btn_pins[] = {
		BTN_UP, BTN_RIGHT, BTN_DOWN, BTN_LEFT, BTN_OK, BTN_CANCEL
	};
	constexpr auto btn_count = sizeof(btn_pins) / sizeof(*btn_pins);

	// Check the state of all button pins and debounce as needed
	for (auto btn_idx = 0U; btn_idx < btn_count; btn_idx++) {
		auto& debouncer = _button_debouncers[btn_idx];
		const auto pin = btn_pins[btn_idx];

		const auto new_event = debouncer.update(digitalRead(pin) == LOW);
		if (new_event == debouncer::event::NONE) {
			continue;
		}

		_notify_new_event(static_cast<id>(btn_idx), static_cast<event>(new_event));
	}
}

nb::watcher::debouncer::debouncer()
{
	transition_to_state(state::NONE);
}

void nb::watcher::debouncer::transition_to_state(nb::watcher::debouncer::state new_state)
{
	_state = new_state;
	_ticks_in_state = 0;
}

nb::watcher::debouncer::event nb::watcher::debouncer::update(bool button_state) noexcept
{
	const auto state_on_entry = _state;

	_ticks_in_state++;

	switch (state_on_entry) {
	case state::NONE:
		if (button_state) {
			transition_to_state(state::DOWN_CANDIDATE);
		}

		break;
	case state::UP_CANDIDATE:
	case state::DOWN_CANDIDATE:
		// Up and down candidate states have values 0 and 1 respectively.
		if (button_state != static_cast<bool>(state_on_entry)) {
			// bouncing detected, reset.
			transition_to_state(state::NONE);
		} else {
			if (_ticks_in_state == nsec::config::button::debounce_ticks - 1) {
				// transition confirmed, trigger and move to next state.
				transition_to_state(state_on_entry == state::UP_CANDIDATE ?
							    state::NONE :
							    state::DOWN_WAIT_FIRST_REPEAT);
				return state_on_entry == state::UP_CANDIDATE ?
					nb::watcher::debouncer::event::UP :
					nb::watcher::debouncer::event::DOWN;
			}
		}

		break;
	case state::DOWN_WAIT_FIRST_REPEAT:
	case state::DOWN_REPEAT:
	{
		constexpr auto ticks_before_first_repeat =
			nsec::config::button::button_down_first_repeat_delay_ms /
			nsec::config::button::polling_period_ms;
		constexpr auto ticks_before_repeat =
			nsec::config::button::button_down_repeat_delay_ms /
			nsec::config::button::polling_period_ms;

		const auto ticks_to_wait = state_on_entry == state::DOWN_WAIT_FIRST_REPEAT ?
			ticks_before_first_repeat :
			ticks_before_repeat;

		if (!button_state) {
			transition_to_state(state::UP_CANDIDATE);
			break;
		}

		if (_ticks_in_state == ticks_to_wait) {
			transition_to_state(state::DOWN_REPEAT);
			return nb::watcher::debouncer::event::DOWN_REPEAT;
		}

		break;
	}
	default:
		// Unreachable or the cosmic rays are hitting pretty hard...
		break;
	}

	return nb::watcher::debouncer::event::NONE;
}