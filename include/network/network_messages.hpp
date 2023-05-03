/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2023 Jérémie Galarneau <jeremie.galarneau@gmail.com>
 */

#ifndef NSEC_NETWORK_MESSAGES_HPP
#define NSEC_NETWORK_MESSAGES_HPP

#include <stdint.h>

#include <ArduinoUniqueID.h>

namespace nsec::communication::message {

enum class type {
	ACK = 0,
	RESET = 1,
	ANNOUNCE_PEER = 2,
	ANNOUNCE_PEER_STOP = 3,
};

struct header {
	uint8_t type;
};

struct announce_peer {
	uint8_t peer_id;
	uint8_t board_unique_id[UniqueIDsize];
};

} // namespace nsec::communication::message

#endif // NSEC_NETWORK_MESSAGES_HPP
