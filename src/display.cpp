// SPDX-FileCopyrightText: 2023 NorthSec
//
// SPDX-License-Identifier: MIT

#include "board.hpp"
#include "display.hpp"

Adafruit_SSD1306 g_display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void nsec::display::init()
{
	g_display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
}
