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
	ANNOUNCE_BADGE_ID =
		nsec::config::communication::application_message_type_range_begin,
};

struct announce_badge_id {
	uint8_t peer_id;
	uint8_t board_unique_id[UniqueIDsize];
} __attribute__((packed));

} // namespace nsec::communication::message

#endif // NSEC_NETWORK_MESSAGES_HPP
