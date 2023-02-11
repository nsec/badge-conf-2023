// SPDX-FileCopyrightText: 2023 NorthSec
//
// SPDX-License-Identifier: MIT

#ifndef NSEC_DISPLAY_HPP
#define NSEC_DISPLAY_HPP

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// WIP: expose our guts
extern Adafruit_SSD1306 g_display;

namespace nsec {
namespace display {
void init(void);
}
} // namespace nsec

#endif /* NSEC_DISPLAY_HPP */
