/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2023 Jérémie Galarneau <jeremie.galarneau@gmail.com>
 */

#include "board.hpp"
#include "globals.hpp"
#include "led/strip_animator.hpp"

namespace nl = nsec::led;
namespace ns = nsec::scheduling;
namespace ng = nsec::g;

namespace {
constexpr int16_t scaling_factor = 1024;

// Linear interpolation in RGB space: not terrible, not great...
nl::strip_animator::led_color interpolate(const nl::strip_animator::keyframe& origin,
					  const nl::strip_animator::keyframe& destination,
					  uint16_t current_time)
{
	nl::strip_animator::led_color new_color;

	if (current_time >= destination.time) {
		return destination.color;
	}

	const int32_t numerator = (int32_t(current_time) - int32_t(origin.time)) * scaling_factor;
	const int32_t denominator =
		(int32_t(destination.time) - int32_t(origin.time)) * scaling_factor;
	for (uint8_t i = 0; i < sizeof(new_color.components); i++) {
		const int32_t origin_component = origin.color.components[i];
		const int32_t destination_component = destination.color.components[i];
		const int32_t total_diff = destination_component - origin_component;
		const int32_t diff_to_apply = ((total_diff * numerator) / denominator);

		new_color.components[i] = uint8_t(origin_component + diff_to_apply);
	}

	return new_color;
}

const nl::strip_animator::keyframe PROGMEM red_to_green_progress_bar_keyframe_template[] = {
	// red
	{ { 10, 0, 0 }, 0 },
	// green <- beginning of loop
	{ { 0, 100, 0 }, nsec::config::badge::pairing_animation_time_per_led_progress_bar_ms },
	// dimmed green to green loop
	{ { 0, 5, 0 }, 5000 },
	{ { 0, 100, 0 }, 6000 },
};

nl::strip_animator::keyframe keyframe_from_flash(const nl::strip_animator::keyframe *src_keyframe)
{
	const auto r = pgm_read_byte(&src_keyframe->color.r());
	const auto g = pgm_read_byte(&src_keyframe->color.g());
	const auto b = pgm_read_byte(&src_keyframe->color.b());

	return { { r, g, b }, pgm_read_word(&src_keyframe->time) };
}

} // namespace

nl::strip_animator::strip_animator() noexcept :
	ns::periodic_task(100) /* Set by the various animations. */,
	_pixels(NUMPIXELS, P_NEOP, NEO_GRB + NEO_KHZ800)
{
	ng::the_scheduler.schedule_task(*this);
}

void nl::strip_animator::setup() noexcept
{
	_pixels.begin();
}

void nl::strip_animator::_legacy_animation_tick() noexcept
{
	if (_state.legacy.start_position > 15) {
		// wrap around when reaching the end led strip
		_state.legacy.start_position = 0;
	}

	// Set all pixel colors to 'off'
	_pixels.clear();
	for (uint8_t i = 0; i < _config.legacy.level; i++) // determine how many LED
							   // should be ON
	{
		// led_ID is the current LED index that we are update.
		uint8_t const led_ID = (_state.legacy.start_position + i) % 16; // This is going
										// over every single
										// LED that needs to
										// be on based on
										// the current LVL

		// enable more colors if your lvl is more than the 16 LEDs
		uint8_t b = 0;
		uint8_t r = 0;
		uint8_t g = 0;
		if (i < 16) // only blue to start
		{
			b = 5;
		} else if (i >= 16 && i < 32) // introduce red
		{
			b = 5;
			r = 5;
		} else if (i >= 32 && i < 48) // introduce green
		{
			b = 5;
			g = 5;
		} else if (i >= 48 && i < 64) // have all 3 colors
		{
			g = 5;
			r = 5;
			b = 5;
		} else if (i >= 64 && i < 80) // increase brightness + speed
		{
			period_ms(100);
			b = 10;
			r = 5;
			b = 5;
		} else if (i >= 80 && i < 96) // increase brightness
		{
			period_ms(50);
			b = 10;
			r = 10;
			b = 5;
		} else if (i >= 96 && i < 112) // increase brightness
		{
			period_ms(25);
			b = 10;
			r = 10;
			b = 10;
		} else if (i >= 112 && i < 128) // max score
		{
			period_ms(12);
			r = random(1, 10);
			g = 0;
			b = 0;
		} else if (i >= 128 && i < 144) // max score
		{
			period_ms(6);
			r = random(1, 10);
			g = 0;
			b = random(1, 10);
		} else if (i >= 144 && i < 160) // max score
		{
			period_ms(3);
			r = random(1, 10);
			g = random(1, 10);
			b = random(1, 10);
		} else if (i >= 160) // max score
		{
			period_ms(1);
			r = random(1, 15);
			g = random(1, 15);
			b = random(1, 15);
		}

		// apply color
		_pixels.setPixelColor(led_ID, _pixels.Color(r, g, b));
	}

	_state.legacy.start_position++;
}

uint8_t nl::strip_animator::_get_keyframe_index(const indice_storage_element *indices,
						uint8_t led_id) const noexcept
{
	const auto& element = indices[led_id / 2];

	return led_id & 1 ? element.odd : element.even;
}

void nl::strip_animator::_set_keyframe_index(indice_storage_element *indices,
					     uint8_t led_id,
					     uint8_t new_keyframe_index) noexcept
{
	auto& element = indices[led_id / 2];

	if (led_id & 1) {
		element.odd = new_keyframe_index;
	} else {
		element.even = new_keyframe_index;
	}
}

void nl::strip_animator::_keyframe_animation_tick(
	const scheduling::absolute_time_ms& current_time_ms) noexcept
{
	// Animation specific setup
	switch (_config.keyframed._animation) {
	default:
		break;
	}

	for (uint8_t i = 0; i < 16; i++) {
		const auto origin_keyframe_index =
			_get_keyframe_index(_state.keyframed._origin_keyframe_index, i);
		const auto destination_keyframe_index =
			_get_keyframe_index(_state.keyframed._destination_keyframe_index, i);

		const auto origin_keyframe =
			keyframe_from_flash(&_config.keyframed.keyframes[origin_keyframe_index]);
		const bool led_animation_is_active = (_config.keyframed.active >> i) & 1;

		if (!led_animation_is_active) {
			// Inactive, repeat the origin keyframe.
			_pixels.setPixelColor(i,
					      origin_keyframe.color.r(),
					      origin_keyframe.color.g(),
					      origin_keyframe.color.b());
			continue;
		}

		// Serial.println();

		const auto time_since_animation_start =
			_state.keyframed.ticks_since_start_of_animation[i] * period_ms();
		// Serial.print(F("Time since animation start: "));
		// Serial.println(time_since_animation_start);

		// origin_keyframe.color.log(F("origin keyframe: "));
		if (_state.keyframed.ticks_since_start_of_animation[i] != 255) {
			// Saturate counter.
			_state.keyframed.ticks_since_start_of_animation[i]++;
		}

		// Interpolate to find the current color.
		const auto destination_keyframe = keyframe_from_flash(
			&_config.keyframed.keyframes[destination_keyframe_index]);
		// destination_keyframe.color.log(F("destination keyframe: "));

		const auto new_color = interpolate(origin_keyframe,
						   destination_keyframe,
						   uint16_t(time_since_animation_start));
		// new_color.log(F("interpolated color: "));


		_pixels.setPixelColor(i,
				      _pixels.gamma8(new_color.r()),
				      _pixels.gamma8(new_color.g()),
				      _pixels.gamma8(new_color.b()));

		// Advance keyframes if needed.
		if (time_since_animation_start < destination_keyframe.time) {
			continue;
		}

		uint8_t new_origin_keyframe_index = destination_keyframe_index;
		uint8_t new_destination_keyframe_index = destination_keyframe_index + 1;
		if (new_destination_keyframe_index >= _config.keyframed.keyframe_count) {
			// Loop back to the configured loop point. Move the current animation time
			// in the past.
			new_origin_keyframe_index = _config.keyframed.loop_point_index;
			new_destination_keyframe_index = min(new_origin_keyframe_index + 1,
							     _config.keyframed.keyframe_count - 1);
			// Round up to make sure the time isn't _before_ the origin keyframe.
			_state.keyframed.ticks_since_start_of_animation[i] =
				((keyframe_from_flash(
					  &_config.keyframed.keyframes[new_origin_keyframe_index])
					  .time +
				  (period_ms() - 1)) /
				 period_ms());
		} else {
			new_origin_keyframe_index = destination_keyframe_index;
			new_destination_keyframe_index = destination_keyframe_index + 1;
		}

		_set_keyframe_index(
			_state.keyframed._origin_keyframe_index, i, new_origin_keyframe_index);
		_set_keyframe_index(_state.keyframed._destination_keyframe_index,
				    i,
				    new_destination_keyframe_index);
	}
}

void nl::strip_animator::run(scheduling::absolute_time_ms current_time_ms) noexcept
{
	switch (_current_animation_type) {
	case animation_type::LEGACY:
		_legacy_animation_tick();
		break;
	case animation_type::KEYFRAMED:
		_keyframe_animation_tick(current_time_ms);
		break;
	default:
		break;
	}

	// Send the updated pixel colors to the hardware.
	_pixels.show();
}

nl::strip_animator::led_color nl::strip_animator::_color(uint8_t led_id) const noexcept
{
	return led_color(&_pixels.getPixels()[led_id * 3]);
}

void nl::strip_animator::set_current_animation_idle(uint8_t current_level) noexcept
{
	period_ms(200);
	_current_animation_type = animation_type::LEGACY;
	_state.legacy.start_position = 0;
	_config.legacy.level = current_level;
}

void nl::strip_animator::_reset_keyframed_animation_state() noexcept
{
	memset(&_state.keyframed, 0, sizeof(_state.keyframed));
}

void nl::strip_animator::set_red_to_green_led_progress_bar(uint8_t active_led_count) noexcept
{
	const bool is_current_animation = _current_animation_type == animation_type::KEYFRAMED &&
		_config.keyframed._animation == keyframed_animation::PROGRESS_BAR;

	if (!is_current_animation) {
		period_ms(40);

		// Setup animation parameters.
		_current_animation_type = animation_type::KEYFRAMED;
		_config.keyframed._animation = keyframed_animation::PROGRESS_BAR;
		_config.keyframed.active = 0;
		_config.keyframed.keyframe_count =
			sizeof(red_to_green_progress_bar_keyframe_template) /
			sizeof(*red_to_green_progress_bar_keyframe_template);
		_config.keyframed.keyframes = red_to_green_progress_bar_keyframe_template;
		_config.keyframed.loop_point_index = 1;

		// Clear its state.
		_reset_keyframed_animation_state();
	}

	for (uint8_t i = 0; i < active_led_count; i++) {
		_config.keyframed.active |= (1 << i);
	}
}
