// SPDX-FileCopyrightText: 2023 NorthSec
//
// SPDX-License-Identifier: MIT

#include "globals.hpp"
#include "ringbuffer.hpp"

void setup()
{
	nsec::g::the_badge.setup();
}

void loop()
{
	nsec::g::the_scheduler.tick(millis());
}
