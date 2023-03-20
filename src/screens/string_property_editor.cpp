/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2023 Jérémie Galarneau <jeremie.galarneau@gmail.com>
 */

#include "display/screen.hpp"
#include "display/string_property_editor.hpp"
#include "display/utils.hpp"
#include "globals.hpp"

namespace nd = nsec::display;
namespace ndu = nsec::display::utils;
namespace nb = nsec::button;
namespace ns = nsec::scheduling;

namespace {
const char how_to_delete_prompt[] PROGMEM = "Press X to delete";
const char how_to_leave_prompt[] PROGMEM = "Hold Okay to exit";

const __FlashStringHelper *as_flash_string(const char *str)
{
	return static_cast<const __FlashStringHelper *>(static_cast<const void *>(str));
}

enum class cycle_character_direction { PREVIOUS, NEXT };

bool is_valid_character(char c)
{
	if (c >= 'A' && c <= 'Z') {
		return true;
	}

	if (c >= 'a' && c <= 'z') {
		return true;
	}

	return c == ' ' || c == '-';
}

char cycle_character(char current_character, cycle_character_direction direction)
{
	const auto direction_operand = direction == cycle_character_direction::NEXT ? 1 : -1;

	do {
		current_character += direction_operand;
	} while (!is_valid_character(current_character));

	return current_character;
}

void delete_character(char *str)
{
	while (*str != '\0') {
		*str = *(str + 1);
		str++;
	}
}
} // anonymous namespace

nd::string_property_editor_screen::prompt_cycle_task::prompt_cycle_task(
	const nsec::callback& action) :
	ns::periodic_task(config::display::prompt_cycle_time), _run{ action }
{
}

void nd::string_property_editor_screen::prompt_cycle_task::run(
	ns::absolute_time_ms current_time) noexcept
{
	_run();
}

nd::string_property_editor_screen::string_property_editor_screen(
	const screen::release_focus_notifier& release_focus_notifier) noexcept :
	screen(release_focus_notifier),
	_prompt{ nullptr },
	_property{},
	_focused_character{ 0 },
	_layout_initialized{ false },
	_prompt_cycle_state{ prompt_cycle_state::PROPERTY_PROMPT },
	_prompt_cycle_task{ { [](void *data) {
				     reinterpret_cast<decltype(this)>(data)->_cycle_prompt();
			     },
			      this } }
{
	nsec::g::the_scheduler.schedule_task(_prompt_cycle_task);
}

void nd::string_property_editor_screen::_move_focused_character(
	nd::string_property_editor_screen::move_direction direction) noexcept
{
	switch (direction) {
	case move_direction::RIGHT:
		if (_focused_character < _property.size - 2) {
			_focused_character++;
		}

		if (_focused_character >= (_first_drawn_character + _edit_characters_per_screen)) {
			_first_drawn_character++;
		}

		break;
	case move_direction::LEFT:
		if (_focused_character > 0) {
			_focused_character--;
		}

		if (_focused_character < _first_drawn_character) {
			_first_drawn_character--;
		}

		break;
	}
}

void nd::string_property_editor_screen::button_event(nb::id id, nb::event event) noexcept
{
	if (event == nb::event::UP) {
		// Not interested in any button release events; it allows us
		// to assume the action is a button press or repeat.
		return;
	}

	// Release focus to the previous screen.
	if (id == nb::id::OK && event == nb::event::DOWN_REPEAT) {
		_release_focus();
	}

	switch (id) {
	case nb::id::RIGHT:
		_move_focused_character(move_direction::RIGHT);
		break;
	case nb::id::LEFT:
		_move_focused_character(move_direction::LEFT);
		break;
	case nb::id::UP:
		_property.value[_focused_character] = cycle_character(
			_property.value[_focused_character], cycle_character_direction::NEXT);
		break;
	case nb::id::DOWN:
		_property.value[_focused_character] = cycle_character(
			_property.value[_focused_character], cycle_character_direction::PREVIOUS);
		break;
	case nb::id::CANCEL:
		delete_character(_property.value + _focused_character);
		_move_focused_character(move_direction::LEFT);
		break;
	default:
		break;
	}

	damage();
}

void nd::string_property_editor_screen::_render(scheduling::absolute_time_ms current_time_ms,
						Adafruit_SSD1306& canvas) noexcept
{
	if (!_layout_initialized) {
		_initialize_layout(canvas);
	}

	canvas.setCursor(0, 0);
	canvas.setTextSize(nsec::config::display::menu_font_size);
	_draw_prompt(canvas);

	canvas.setCursor(_edit_character_x_offset, _edit_character_y_offset);
	canvas.setTextSize(2);
	ndu::draw_string(canvas,
			 _property.value + _first_drawn_character,
			 _edit_characters_per_screen,
			 false);

	// Redraw highlighted character over property string
	canvas.fillRect(
		_edit_character_x_offset - 1 +
			((_focused_character - _first_drawn_character) * _edit_character_width),
		canvas.getCursorY(),
		_edit_character_width,
		_edit_character_height,
		SSD1306_WHITE);
	canvas.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
	canvas.setCursor(
		_edit_character_x_offset +
			((_focused_character - _first_drawn_character) * _edit_character_width),
		canvas.getCursorY());
	canvas.write(_property.value[_focused_character]);
	canvas.setTextColor(SSD1306_WHITE, SSD1306_BLACK);

	// draw right arrow
	if (static_cast<unsigned int>(_first_drawn_character + _edit_characters_per_screen) <
	    _property.size - 1) {
		ndu::draw_arrow_glyph(canvas,
				      (_edit_character_width * _edit_characters_per_screen) +
					      _edit_character_x_offset,
				      _edit_character_y_offset + (_edit_character_height / 4),
				      _prompt_glyph_width,
				      _prompt_glyph_height,
				      SSD1306_WHITE,
				      ndu::arrow_glyph_direction::RIGHT);
	}

	// draw left arrow
	if (_first_drawn_character != 0) {
		ndu::draw_arrow_glyph(canvas,
				      0,
				      _edit_character_y_offset + (_edit_character_height / 4),
				      _prompt_glyph_width,
				      _prompt_glyph_height,
				      SSD1306_WHITE,
				      ndu::arrow_glyph_direction::LEFT);
	}
}

void nd::string_property_editor_screen::set_property(const __FlashStringHelper *prompt,
						     char *property,
						     size_t property_size) noexcept
{
	_prompt = prompt;
	_property = { property, property_size };

	bool found_null = false;

	// Convert tail end of string property to spaces instead of nulls.
	for (size_t i = 0; i < property_size; i++) {
		found_null |= property[i] == '\0';
		if (found_null) {
			property[i] = ' ';
			continue;
		}
	}

	property[property_size - 1] = '\0';
}

void nd::string_property_editor_screen::focused() noexcept
{
	_first_drawn_character = 0;
	_focused_character = 0;
}

void nd::string_property_editor_screen::clean_up_property() noexcept
{
	for (size_t i = _property.size - 1; i >= 0; i--) {
		if (_property.value[i] == ' ') {
			_property.value[i] = '\0';
		} else {
			break;
		}
	}
}

void nd::string_property_editor_screen::_initialize_layout(Adafruit_SSD1306& canvas) noexcept
{
	int x, y;
	unsigned int text_width, text_height;

	canvas.setTextSize(1);
	canvas.getTextBounds("a", 0, 0, &x, &y, &text_width, &text_height);
	_prompt_characters_per_screen = canvas.width() / text_width;
	_edit_character_y_offset = text_height + (text_height / 2);
	_prompt_glyph_width = text_width;
	_prompt_glyph_height = text_height;

	canvas.setTextSize(2);
	canvas.getTextBounds("a", 0, 0, &x, &y, &text_width, &text_height);
	_edit_characters_per_screen = (canvas.width() / text_width) - 1;
	_edit_character_width = text_width;
	_edit_character_height = text_height;
	_edit_character_x_offset = text_width / 2;

	_layout_initialized = true;
}

void nd::string_property_editor_screen::_draw_prompt(Adafruit_SSD1306& canvas) noexcept
{
	switch (_prompt_cycle_state) {
	case prompt_cycle_state::PROPERTY_PROMPT:
		ndu::draw_string(canvas, _prompt, _prompt_characters_per_screen);
		break;
	case prompt_cycle_state::HOW_TO_DELETE:
		ndu::draw_string(canvas,
				 as_flash_string(how_to_delete_prompt),
				 _prompt_characters_per_screen);
		break;
	case prompt_cycle_state::HOW_TO_QUIT:
		ndu::draw_string(canvas,
				 as_flash_string(how_to_leave_prompt),
				 _prompt_characters_per_screen);
		break;
	default:
		return;
	}
}

void nd::string_property_editor_screen::_cycle_prompt() noexcept
{
	// Periodically invoked by the prompt cycle task.
	const auto current_state = static_cast<int>(_prompt_cycle_state);
	const auto next_state = static_cast<prompt_cycle_state>(current_state + 1);

	_prompt_cycle_state = next_state == prompt_cycle_state::END ?
		prompt_cycle_state::PROPERTY_PROMPT :
		next_state;

	// The new prompt will be rendered on the next tick.
	damage();
}