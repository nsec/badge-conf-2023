/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2023 Jérémie Galarneau <jeremie.galarneau@gmail.com>
 */

#include "board.hpp"
#include "config.hpp"
#include "network/network_handler.hpp"

namespace ns = nsec::scheduling;
namespace nc = nsec::communication;

nc::network_handler::network_handler(pairing_begin pairing_begin,
				     pairing_end pairing_end,
				     message_received message_received) noexcept :
	ns::periodic_task(nsec::config::communication::network_handler_base_period_ms),
	_notify_pairing_begin{ pairing_begin },
	_notify_pairing_end{ pairing_end },
	_notify_message_received{ message_received },
	_left_Serial(nsec::config::communication::serial_rx_pin_left,
		     nsec::config::communication::serial_tx_pin_left),
	_right_Serial(nsec::config::communication::serial_rx_pin_right,
		      nsec::config::communication::serial_tx_pin_right),
	_rx_expected_size_left(0),
	_rx_expected_size_right(0),
	_topology_role{ topology_role::UNCONNECTED }
{
}

void nc::network_handler::setup() noexcept
{
	_left_Serial.begin(nsec::config::communication::software_serial_speed);
	_right_Serial.begin(nsec::config::communication::software_serial_speed);
}

void nc::network_handler::run(ns::absolute_time_ms current_time_ms) noexcept
{
}