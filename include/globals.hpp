// SPDX-FileCopyrightText: 2023 NorthSec
//
// SPDX-License-Identifier: MIT

#ifndef NSEC_GLOBALS_HPP
#define NSEC_GLOBALS_HPP

#include "stdint.h"

namespace nsec {
namespace g {
extern uint8_t currentLevel;
extern bool currentlyLoading;
extern bool waitingForDisconnect;
extern uint8_t loadingBarPos;
} // namespace g
} // namespace nsec

#endif /* NSEC_GLOBALS_HPP */
