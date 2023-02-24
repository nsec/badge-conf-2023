// SPDX-FileCopyrightText: 2023 NorthSec
//
// SPDX-License-Identifier: MIT

#include "globals.hpp"

uint8_t nsec::g::currentLevel = 1;
bool nsec::g::currentlyLoading = false;
bool nsec::g::waitingForDisconnect = false;
uint8_t nsec::g::loadingBarPos = 0;
