/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2023 Jérémie Galarneau <jeremie.galarneau@gmail.com>
 */

#ifndef NSEC_CONFIG_HPP
#define NSEC_CONFIG_HPP

#include "scheduler.hpp"

#include <stdint.h>

namespace nsec::config::scheduler {
constexpr unsigned int max_scheduled_task_count = 16;
}

namespace nsec::config::social {
constexpr uint8_t max_level = 200;
}

namespace nsec::config::button {
constexpr nsec::scheduling::relative_time_ms polling_period_ms = 10;

// How many ticks a button's state must be observed in to trigger.
constexpr uint8_t debounce_ticks = 2;

// Must be multiples of the polling period.
constexpr nsec::scheduling::relative_time_ms button_down_first_repeat_delay_ms = 500;
static_assert(button_down_first_repeat_delay_ms % polling_period_ms == 0);

constexpr nsec::scheduling::relative_time_ms button_down_repeat_delay_ms = 30;
static_assert(button_down_repeat_delay_ms % polling_period_ms == 0);

}; // namespace nsec::config::button

namespace nsec::config::display {
// Gives a very cinematic 5 fps
constexpr nsec::scheduling::relative_time_ms refresh_period_ms = 200;
} // namespace nsec::config::display

#endif // NSEC_CONFIG_HPP
