/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2023 Jérémie Galarneau <jeremie.galarneau@gmail.com>
 */

#include "display/splash.hpp"
#include "display/utils.hpp"
#include "globals.hpp"
#include "logo.hpp"

namespace nd = nsec::display;
namespace nb = nsec::button;
namespace ns = nsec::scheduling;

nd::splash_screen::splash_screen() noexcept : screen()
{
}

void nd::splash_screen::button_event(nb::id id, nb::event event) noexcept
{
}

void nd::splash_screen::_render(scheduling::absolute_time_ms current_time_ms [[maybe_unused]],
				Adafruit_SSD1306& canvas) noexcept
{
	canvas.drawBitmap(0, 0, nsec_logo_data, canvas.width(), canvas.height(), 1);
}

void nd::splash_screen::one_shot_timer_task::run(ns::absolute_time_ms current_time
						 [[maybe_unused]]) noexcept
{
	nsec::g::the_badge.relase_focus_current_screen();
}

void nd::splash_screen::focused() noexcept
{
	nsec::g::the_scheduler.schedule_task(_timer,
					     nsec::config::splash::splash_screen_wait_time_ms);
	nd::screen::focused();
}