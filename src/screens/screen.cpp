/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2023 Jérémie Galarneau <jeremie.galarneau@gmail.com>
 */

#include "display/screen.hpp"
#include "globals.hpp"

nsec::display::screen::screen() noexcept : _cleared_on_every_frame{ true }
{
}

void nsec::display::screen::_release_focus() noexcept
{
	nsec::g::the_badge.relase_focus_current_screen();
}