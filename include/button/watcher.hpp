/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2023 Jérémie Galarneau <jeremie.galarneau@gmail.com>
 */

#ifndef NSEC_BUTTON_WATCHER_HPP
#define NSEC_BUTTON_WATCHER_HPP

#include <scheduler.hpp>
#include <stdint.h>

namespace nsec::button {

enum class id { UP = 0, RIGHT = 1, DOWN = 2, LEFT = 3, OK = 4, CANCEL = 5 };
enum class event { UP, DOWN, DOWN_REPEAT };

/*
 * Callable provided to a button watcher and invoked when a new button event is
 * detected.
 */
class new_button_event_notifier {
public:
	using new_button_event_cb = void (*)(id id, event event, void *data);

	new_button_event_notifier(new_button_event_cb function, void *data) noexcept :
		_function{ function }, _user_data{ data }
	{
	}

	void operator()(id id, event event) const
	{
		_function(id, event, _user_data);
	}

private:
	new_button_event_cb _function;
	void *_user_data;
};

/*
 * Tracks the state of the badge's buttons to debounce and transform
 * the pin readings into UI button events.
 */
class watcher : public nsec::scheduling::periodic_task {
public:
	explicit watcher(new_button_event_notifier new_button_notifier) noexcept;

	// Deactivate copy and assignment.
	watcher(const watcher&) = delete;
	watcher(watcher&&) = delete;
	watcher& operator=(const watcher&) = delete;
	watcher& operator=(watcher&&) = delete;
	~watcher() override = default;

	// Setup hardware.
	void setup() noexcept;

protected:
	void run(scheduling::absolute_time_ms current_time_ms) noexcept override;

private:
	class debouncer {
	public:
		debouncer();

		// Deactivate copy and assignment.
		debouncer(const debouncer&) = delete;
		debouncer(debouncer&&) = delete;
		debouncer& operator=(const debouncer&) = delete;
		debouncer& operator=(debouncer&&) = delete;
		~debouncer() = default;

		enum class event {
			NONE = -1,
			UP = static_cast<int>(button::event::UP),
			DOWN = static_cast<int>(button::event::DOWN),
			DOWN_REPEAT = static_cast<int>(button::event::DOWN_REPEAT),
		};

		event update(bool button_state) noexcept;

	private:
		enum class state : uint8_t {
			UP_CANDIDATE = 0,
			DOWN_CANDIDATE = 1,
			DOWN_WAIT_FIRST_REPEAT = 2,
			DOWN_REPEAT = 3,
			NONE = 4,
		};

		void transition_to_state(state new_state);

		/*
		 * _state contains values from `state`. However storing state as a 4-bit bitfield
		 * results in a nasty warning that not all values of 'state' fit in 4 bits.
		 */
		uint8_t _state;
		// Ticks since transition into state
		uint8_t _ticks_in_state;
	} _button_debouncers[static_cast<size_t>(id::CANCEL) + 1];
	new_button_event_notifier _notify_new_event;
};

} // namespace nsec::button

#endif // NSEC_BUTTON_WATCHER_HPP