/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2023 Jérémie Galarneau <jeremie.galarneau@gmail.com>
 */

#ifndef NSEC_DISPLAY_STRING_PROPERTY_EDITOR_SCREEN_HPP
#define NSEC_DISPLAY_STRING_PROPERTY_EDITOR_SCREEN_HPP

#include "config.hpp"
#include "display/screen.hpp"
#include "scheduler.hpp"

namespace nsec::display {

class string_property_editor_screen : public screen {
public:
	explicit string_property_editor_screen(
		const screen::release_focus_notifier& release_focus_notifier) noexcept;

	/* Deactivate copy and assignment. */
	string_property_editor_screen(const string_property_editor_screen&) = delete;
	string_property_editor_screen(string_property_editor_screen&&) = delete;
	string_property_editor_screen& operator=(const string_property_editor_screen&) = delete;
	string_property_editor_screen& operator=(string_property_editor_screen&&) = delete;
	~string_property_editor_screen() override = default;

	void button_event(button::id id, button::event event) noexcept override;
	void _render(scheduling::absolute_time_ms current_time_ms,
		     Adafruit_SSD1306& canvas) noexcept override;

	void set_property(const __FlashStringHelper *prompt,
			  char *property,
			  size_t property_size) noexcept;

	void focused() noexcept override;

	// Clean-up the current property (make it null-terminated).
	void clean_up_property();

private:
	enum class move_direction { LEFT, RIGHT };
	enum class prompt_cycle_state : uint8_t {
		PROPERTY_PROMPT = 0,
		HOW_TO_DELETE = 1,
		HOW_TO_QUIT = 2,
		END = 3
	};

	class prompt_cycle_task : public nsec::scheduling::periodic_task {
	public:
		explicit prompt_cycle_task(const nsec::callback& action);
		void run(nsec::scheduling::absolute_time_ms current_time) noexcept override;

	private:
		nsec::callback _run;
	};

	void _initialize_layout(Adafruit_SSD1306& canvas) noexcept;
	void _draw_prompt(Adafruit_SSD1306& canvas) noexcept;
	void _cycle_prompt() noexcept;
	void _move_focused_character(move_direction direction) noexcept;

	const __FlashStringHelper *_prompt;
	struct {
		char *value;
		size_t size;
	} _property;

	uint8_t _focused_character;
	uint8_t _first_drawn_character;
	bool _layout_initialized : 1;
	unsigned int _prompt_characters_per_screen : 5;
	unsigned int _edit_characters_per_screen : 5;
	unsigned int _edit_character_width : 5;
	unsigned int _edit_character_height : 5;
	unsigned int _edit_character_y_offset : 5;
	unsigned int _edit_character_x_offset : 3;
	unsigned int _prompt_glyph_width : 4;
	unsigned int _prompt_glyph_height : 4;

	prompt_cycle_state _prompt_cycle_state;
	prompt_cycle_task _prompt_cycle_task;
};
} // namespace nsec::display

#endif // NSEC_DISPLAY_STRING_PROPERTY_EDITOR_SCREEN_HPP
