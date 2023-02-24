// SPDX-FileCopyrightText: 2023 NorthSec
//
// SPDX-License-Identifier: MIT

#ifndef NSEC_GLOBALS_HPP
#define NSEC_GLOBALS_HPP

#include "stdint.h"
#include "badge.hpp"
#include "config.hpp"

namespace nsec::g {
extern uint8_t currentLevel;
extern bool currentlyLoading;
extern bool waitingForDisconnect;
extern uint8_t loadingBarPos;

extern runtime::badge the_badge;
} // namespace nsec::g

#endif /* NSEC_GLOBALS_HPP */
