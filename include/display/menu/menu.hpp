/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2023 Jérémie Galarneau <jeremie.galarneau@gmail.com>
 */

#ifndef NSEC_DISPLAY_MENU_SCREEN_HPP
#define NSEC_DISPLAY_MENU_SCREEN_HPP

#include "display/screen.hpp"
#include "callback.hpp"

namespace nsec::display {

class menu_screen : public screen {
public:
	class choices {
	public:
		class choice {
		public:
			using menu_choice_action = nsec::callback<void>;

			choice(const __FlashStringHelper *name, const menu_choice_action& action) :
				name{ name }, _action{ action }
			{
			}

			/* Deactivate copy and assignment. */
			choice(const choice&) = delete;
			choice(choice&&) = delete;
			choice& operator=(const choice&) = delete;
			choice& operator=(choice&&) = delete;
			~choice() = default;

			void operator()() const
			{
				_action();
			}

			const __FlashStringHelper *name = nullptr;

		private:
			const menu_choice_action _action;
		};

		choices() noexcept = default;

		/* Deactivate copy and assignment. */
		choices(const choices&) = delete;
		choices(choices&&) = delete;
		choices& operator=(const choices&) = delete;
		choices& operator=(choices&&) = delete;
		~choices() = default;

		virtual uint8_t count() const noexcept = 0;
		virtual const choice& operator[](uint8_t) const noexcept = 0;
	};

	explicit menu_screen() noexcept;

	/* Deactivate copy and assignment. */
	menu_screen(const menu_screen&) = delete;
	menu_screen(menu_screen&&) = delete;
	menu_screen& operator=(const menu_screen&) = delete;
	menu_screen& operator=(menu_screen&&) = delete;

	void button_event(button::id id, button::event event) noexcept override;
	void _render(scheduling::absolute_time_ms current_time_ms,
		    Adafruit_SSD1306& canvas) noexcept override;

	void set_choices(const choices& new_choices) noexcept
	{
		_choices = &new_choices;
		_layout_constraints_initialized = false;
		_active_choice_idx = 0;
		_first_drawn_choice_index = 0;
	}

private:
	void _initialize_layout_constraints(Adafruit_SSD1306& canvas) noexcept;
	bool _is_choice_offscreen(unsigned int choice) const noexcept;

	const choices *_choices = nullptr;
	bool _layout_constraints_initialized : 1;
	struct {
		struct {
			unsigned int width : 5;
			unsigned int height : 5;
		} glyph_size;
		unsigned int _lines_per_screen:4;
		unsigned int _chars_per_screen:5;
	} _layout_constraints;
	unsigned int _active_choice_idx : 7;
	unsigned int _first_drawn_choice_index:7;
};
} // namespace nsec::display

#endif // NSEC_DISPLAY_MENU_SCREEN_HPP
