/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2023 Jérémie Galarneau <jeremie.galarneau@gmail.com>
 */

#ifndef NSEC_NETWORK_MESSAGES_HPP
#define NSEC_NETWORK_MESSAGES_HPP

#include "config.hpp"

#include <ArduinoUniqueID.h>
#include <stdint.h>

namespace nsec::communication::message {

enum class type {
	SOME_APPLICATION_MESSAGE_TYPE =
		nsec::config::communication::application_message_type_range_begin,
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
