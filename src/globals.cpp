// SPDX-FileCopyrightText: 2023 NorthSec
//
// SPDX-License-Identifier: MIT

#include "globals.hpp"

bool nsec::g::currentlyLoading = false;
bool nsec::g::waitingForDisconnect = false;
uint8_t nsec::g::loadingBarPos = 0;
nsec::scheduling::scheduler<nsec::config::scheduler::max_scheduled_task_count>
	nsec::g::the_scheduler;
nsec::runtime::badge nsec::g::the_badge;
