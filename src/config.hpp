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
constexpr uint8_t max_level = 128;
}

namespace nsec::config::user {
constexpr uint8_t name_max_length = 32;
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
constexpr nsec::scheduling::relative_time_ms refresh_period_ms = 4;

constexpr uint8_t menu_font_size = 1;
constexpr uint8_t scroll_font_size = 3;

constexpr nsec::scheduling::absolute_time_ms prompt_cycle_time = 2000;

constexpr uint8_t scroll_pixels_per_second = 80;
} // namespace nsec::config::display

namespace nsec::config::communication {
// Size reserved for protocol messages (without any encryption/signing overhead)
constexpr size_t protocol_max_message_size = 16;
constexpr unsigned int software_serial_speed = 38400;
/*
* Applications may define messages >= application_message_type_range_begin.
* IDs under this range are reserved by the wire protocol.
*/
constexpr uint8_t application_message_type_range_begin = 10;

constexpr unsigned int connection_sense_pin_left = SIG_L3;
constexpr unsigned int connection_sense_pin_right = SIG_R3;

constexpr unsigned int serial_rx_pin_left = SIG_L1;
constexpr unsigned int serial_tx_pin_left = SIG_L2;
constexpr unsigned int serial_rx_pin_right = SIG_R2;
constexpr unsigned int serial_tx_pin_right = SIG_R1;

constexpr nsec::scheduling::relative_time_ms network_handler_base_period_ms = 100;
constexpr nsec::scheduling::relative_time_ms network_handler_timeout_ms = 15000;

} // namespace nsec::communication

#endif // NSEC_CONFIG_HPP
