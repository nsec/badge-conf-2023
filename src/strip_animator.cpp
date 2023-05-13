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
	{ { 100, 20, 0 }, 0 },
	{ { 0, 0, 0 }, nsec::config::badge::pairing_animation_time_per_led_progress_bar_ms / 2 },
	// green <- beginning of loop
	{ { 0, 255, 0 }, nsec::config::badge::pairing_animation_time_per_led_progress_bar_ms },
	// dimmed green to green loop
	{ { 48, 120, 19 },
	  nsec::config::badge::pairing_animation_time_per_led_progress_bar_ms * 2 },
	{ { 0, 255, 0 }, nsec::config::badge::pairing_animation_time_per_led_progress_bar_ms * 3 },
};

const nl::strip_animator::keyframe PROGMEM happy_clown_barf_keyframes[] = {
	{ { 0, 0, 0 }, 0 },
	{ { 161, 255, 181 }, 100 },
	{ { 255, 128, 140 }, 200 },
	{ { 255, 188, 110 }, 300 },
	{ { 255, 255, 110 }, 400 },
	{ { 110, 192, 255 }, 500 },
	{ { 161, 255, 181 }, 600 },
};

const nl::strip_animator::keyframe PROGMEM no_new_friends_keyframes[] = {
	{ { 0, 0, 0 }, 0 },
	{ { 255, 0, 0 }, 200 },
	{ { 0, 0, 0 }, 400 },
	{ { 255, 0, 0 }, 600 },
};

// Colors evoking a star cooling down
const nl::strip_animator::keyframe PROGMEM burning_star_keyframes[] = {
	{ { 0, 0, 0 }, 0 }, // black
	{ { 255, 255, 255 }, 100 }, // white
	{ { 180, 0, 200 }, 400 }, // violet
	{ { 70, 0, 0 }, 900 }, // red
	{ { 0, 0, 0 }, 1500 }, // black
	{ { 0, 0, 0 }, 5000 }, // maintain black
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
	case keyframed_animation::SHOOTING_STAR:
		if (_state.keyframed.shooting_star.ticks_in_position ==
		    _config.keyframed.shooting_star.ticks_before_advance) {
			_state.keyframed.shooting_star.ticks_in_position = 0;
			// Stored on 4 bits, wraps around at 15.
			_state.keyframed.shooting_star.position++;

			const auto led_interval = 16 / _config.keyframed.shooting_star.star_count;
			for (uint8_t i = 0; i < _config.keyframed.shooting_star.star_count; i++) {
				const auto position = (_state.keyframed.shooting_star.position +
						       (led_interval * i)) %
					16;

				_set_keyframe_index(
					_state.keyframed.origin_keyframe_index, position, 0);
				_set_keyframe_index(
					_state.keyframed.destination_keyframe_index, position, 0);

				_state.keyframed.ticks_since_start_of_animation[position] = 0;
				_config.keyframed.active |= 1 << position;
			}
		}

		_state.keyframed.shooting_star.ticks_in_position++;
		break;
	default:
		break;
	}

	_pixels.setBrightness(_config.keyframed.brightness);

	for (uint8_t i = 0; i < 16; i++) {
		const auto origin_keyframe_index =
			_get_keyframe_index(_state.keyframed.origin_keyframe_index, i);
		const auto destination_keyframe_index =
			_get_keyframe_index(_state.keyframed.destination_keyframe_index, i);

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

		const auto time_since_animation_start =
			_state.keyframed.ticks_since_start_of_animation[i] * period_ms();

		//	Serial.println();
		//	Serial.print(F("Time since animation start: "));
		//	Serial.println(time_since_animation_start);
		//	origin_keyframe.color.log(F("origin keyframe: "));

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
			_state.keyframed.origin_keyframe_index, i, new_origin_keyframe_index);
		_set_keyframe_index(_state.keyframed.destination_keyframe_index,
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
		_config.keyframed.loop_point_index = 2;
		_config.keyframed.brightness = 120;

		// Clear its state.
		_reset_keyframed_animation_state();
	}

	for (uint8_t i = 0; i < active_led_count; i++) {
		_config.keyframed.active |= (1 << i);
	}
}

void nl::strip_animator::set_pairing_completed_animation(
	nl::strip_animator::pairing_completed_animation_type animation_type) noexcept
{
	set_show_level_animation(animation_type, 0xFF, true);
}

void nl::strip_animator::set_show_level_animation(
	nl::strip_animator::pairing_completed_animation_type animation_type,
	uint8_t level,
	bool set_lower_bar_on) noexcept
{
	period_ms(40);

	uint8_t cycle_offset = 0;
	uint16_t active_mask = 0;

	for (uint8_t i = 0; i < 8; i++) {
		// LED at bit number one is the left-most, so we need to "invert" the level pattern.
		const auto value_bit = (level >> i) & 1;
		active_mask |= value_bit << (7 - i);
	}

	const keyframe *keyframes = nullptr;
	uint8_t keyframe_count = 0;

	if (animation_type == pairing_completed_animation_type::HAPPY_CLOWN_BARF) {
		keyframe_count =
			sizeof(happy_clown_barf_keyframes) / sizeof(*happy_clown_barf_keyframes);
		keyframes = happy_clown_barf_keyframes;
		// Apply a slight offset between LEDs to achieve a "sparkle" effect.
		cycle_offset = 10;
	} else {
		keyframe_count =
			sizeof(no_new_friends_keyframes) / sizeof(*no_new_friends_keyframes);
		keyframes = no_new_friends_keyframes;
	}

	if (set_lower_bar_on) {
		active_mask |= 0xFF00;
	}

	_set_keyframed_cycle_animation(keyframes, keyframe_count, 1, active_mask, cycle_offset, 40);
}

void nl::strip_animator::set_shooting_star_animation(uint8_t star_count,
						     unsigned int advance_interval_ms) noexcept
{
	period_ms(20);
	_current_animation_type = animation_type::KEYFRAMED;
	_config.keyframed._animation = keyframed_animation::SHOOTING_STAR;
	_config.keyframed.active = 0;
	_reset_keyframed_animation_state();
	_config.keyframed.keyframe_count =
		sizeof(burning_star_keyframes) / sizeof(*burning_star_keyframes);
	_config.keyframed.keyframes = burning_star_keyframes;

	_config.keyframed.loop_point_index = 0;
	_config.keyframed.brightness = 50;
	_config.keyframed.shooting_star.ticks_before_advance = advance_interval_ms / period_ms();
	_config.keyframed.shooting_star.star_count = star_count;
}

void nl::strip_animator::_set_keyframed_cycle_animation(const keyframe *keyframe,
							uint8_t keyframe_count,
							uint8_t loop_point_index,
							uint16_t active_mask,
							uint8_t cycle_offset_between_frames,
							uint8_t refresh_rate) noexcept
{
	period_ms(refresh_rate);

	// Setup animation parameters.
	_current_animation_type = animation_type::KEYFRAMED;
	_config.keyframed._animation = keyframed_animation::CYCLE;
	_config.keyframed.active = active_mask;

	_reset_keyframed_animation_state();

	_config.keyframed.keyframe_count = keyframe_count;
	_config.keyframed.keyframes = keyframe;

	// Apply an offset between LEDs to achieve a "sparkle" effect.
	for (uint8_t i = 0; i < 16; i++) {
		_state.keyframed.ticks_since_start_of_animation[i] =
			(i * cycle_offset_between_frames) %
			(keyframe_from_flash(
				 &_config.keyframed.keyframes[_config.keyframed.keyframe_count - 1])
				 .time /
			 period_ms());
	}

	_config.keyframed.loop_point_index = loop_point_index;
	_config.keyframed.brightness = 50;
}