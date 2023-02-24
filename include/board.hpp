// SPDX-FileCopyrightText: 2023 NorthSec
//
// SPDX-License-Identifier: MIT

#ifndef NSEC_BOARD_HPP
#define NSEC_BOARD_HPP

/*
 * LEDs
 */
#define LED_DBG 13

/*
 * Buttons
 */
#define BTN_UP	  A1
#define BTN_DOWN  A2
#define BTN_LEFT  A3
#define BTN_RIGHT A0
#define BTN_OK    A7
#define BTN_CANCEL A6

/*
 * NeoPixels
 */
#define P_NEOP	  9
#define NUMPIXELS 16

/*
 * OLED Screen
 */
#define SCREEN_WIDTH  128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS \
	0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
	     ///
/*
 * Software serial ports
 */
#define SIG_L1 2
#define SIG_L2 3
#define SIG_L3 4
#define SIG_R1 5
#define SIG_R2 6
#define SIG_R3 7

#endif /* NSEC_BOARD_HPP */
