/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2023 Jérémie Galarneau <jeremie.galarneau@gmail.com>
 */

#ifndef NSEC_LED_STRIP_ANIMATOR_HPP
#define NSEC_LED_STRIP_ANIMATOR_HPP

#include "scheduler.hpp"

#include <Adafruit_NeoPixel.h>

namespace nsec::led {

class strip_animator : public scheduling::periodic_task {
public:
	strip_animator() noexcept;

	/* Deactivate copy and assignment. */
	strip_animator(const strip_animator&) = delete;
	strip_animator(strip_animator&&) = delete;
	strip_animator& operator=(const strip_animator&) = delete;
	strip_animator& operator=(strip_animator&&) = delete;
	~strip_animator() = default;

	void setup() noexcept;

	void set_current_animation_idle(unsigned int current_level) noexcept;
	void set_breathing_led_count(unsigned int led_count) noexcept;

protected:
	void run(scheduling::absolute_time_ms current_time_ms) noexcept override;

private:
	enum class animation_type {
		LEGACY,
	};

	void _legacy_animation_tick() noexcept;

	Adafruit_NeoPixel _pixels;
	animation_type _current_animation;

	union {
		struct {
			uint8_t level;
		} legacy;
	} _animation_config;
	union {
		struct {
			uint8_t start_position;

		} legacy;
	} _animation_state;
};
} // namespace nsec::led
#endif // NSEC_LED_STRIP_ANIMATOR_HPP
