#ifndef NSEC_NEOPIXEL_HPP
#define NSEC_NEOPIXEL_HPP

#include <Adafruit_NeoPixel.h>

void neopix_init(void);
void neopix_idle(void);
void neopix_connectLeft(void);
void neopix_connectRight(void);
void neopix_success(void);

// WIP: expose our guts
extern Adafruit_NeoPixel g_pixels;

#endif /* NSEC_NEOPIXEL_HPP */
