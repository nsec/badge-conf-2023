/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2023 Jérémie Galarneau <jeremie.galarneau@gmail.com>
 */

#ifndef NSEC_CONFIG_HPP
#define NSEC_CONFIG_HPP

#include "board.hpp"
#include "scheduler.hpp"

#include <stdint.h>

namespace nsec::config::scheduler {
constexpr unsigned int max_scheduled_task_count = 16;
}

namespace nsec::config::social {
constexpr uint8_t max_level = 200;
}

namespace nsec::config::user {
constexpr uint8_t name_max_length = 64;
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

} // namespace nsec::config::button

namespace nsec::config::display {
// Gives a very cinematic ~16 fps
constexpr nsec::scheduling::relative_time_ms refresh_period_ms = 60;

constexpr unsigned int menu_font_size = 1;

constexpr nsec::scheduling::absolute_time_ms prompt_cycle_time = 2000;
} // namespace nsec::config::display

namespace nsec::config::communication {
// Size reserved for user protocol messages (without any encryption/signing overhead)
constexpr size_t protocol_buffer_size = 24;
constexpr unsigned int software_serial_speed = 4800;

constexpr unsigned int connection_sense_pin_left = SIG_L3;
constexpr unsigned int connection_sense_pin_right = SIG_R2;

constexpr unsigned int serial_rx_pin_left = SIG_L1;
constexpr unsigned int serial_tx_pin_left = SIG_L2;
constexpr unsigned int serial_rx_pin_right = SIG_R2;
constexpr unsigned int serial_tx_pin_right = SIG_R1;

constexpr nsec::scheduling::relative_time_ms network_handler_base_period_ms = 100;

} // namespace nsec::communication

#endif // NSEC_CONFIG_HPP
