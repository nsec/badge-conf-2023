/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2023 Jérémie Galarneau <jeremie.galarneau@gmail.com>
 */

#ifndef NSEC_NETWORK_HANDLER_HPP
#define NSEC_NETWORK_HANDLER_HPP

#include "callback.hpp"
#include "config.hpp"
#include "network_messages.hpp"
#include "scheduler.hpp"

#include <SoftwareSerial.h>

namespace nsec::communication {

enum class peer_relative_position {
	LEFT,
	RIGHT,
};

class network_handler : public scheduling::periodic_task {
public:
	using pairing_begin = nsec::callback<void>;
	// void (our_peer_id, peer count)
	using pairing_end = nsec::callback<void, unsigned int>;
	// void (relative_position_of_peer, message_type, message_payload)
	using message_received = nsec::callback<void, peer_relative_position, nsec::communication::message::type, uint8_t *>;

	network_handler(pairing_begin, pairing_end, message_received) noexcept;

	/* Deactivate copy and assignment. */
	network_handler(const network_handler&) = delete;
	network_handler(network_handler&&) = delete;
	network_handler& operator=(const network_handler&) = delete;
	network_handler& operator=(network_handler&&) = delete;
	~network_handler() override = default;

	void setup() noexcept;

protected:
	void run(scheduling::absolute_time_ms current_time_ms) noexcept override;

private:
	enum class topology_role { UNCONNECTED = 0, LEFT_MOST = 1, RIGHT_MOST = 2, MIDDLE = 3 };

	// Event handlers
	pairing_begin _notify_pairing_begin;
	pairing_end _notify_pairing_end;
	message_received _notify_message_received;

	SoftwareSerial _left_Serial;
	SoftwareSerial _right_Serial;

	size_t _rx_expected_size_left : 7;
	size_t _rx_expected_size_right : 7;
	topology_role _topology_role;
};
} // namespace nsec::communication

#endif // NSEC_NETWORK_HANDLER_HPP
