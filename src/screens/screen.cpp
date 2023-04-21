/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2023 Jérémie Galarneau <jeremie.galarneau@gmail.com>
 */

#include "display/screen.hpp"
#include "globals.hpp"

void nsec::display::screen::_release_focus() noexcept
{
	nsec::g::the_badge.relase_focus_current_screen();
}