/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2023 Jérémie Galarneau <jeremie.galarneau@gmail.com>
 */

#ifndef NSEC_DISPLAY_MAIN_MENU_CHOICES_HPP
#define NSEC_DISPLAY_MAIN_MENU_CHOICES_HPP

#include "display/menu/menu.hpp"

namespace nsec::display {

class main_menu_choices : public menu_screen::choices {
public:
	explicit main_menu_choices(
		const choices::choice::menu_choice_action& set_name_action) noexcept;

	/* Deactivate copy and assignment. */
	main_menu_choices(const main_menu_choices&) = delete;
	main_menu_choices(main_menu_choices&&) = delete;
	main_menu_choices& operator=(const main_menu_choices&) = delete;
	main_menu_choices& operator=(main_menu_choices&&) = delete;
	~main_menu_choices() override = default;

	unsigned int count() const noexcept override;
	const choice& operator[](unsigned int) const noexcept override;

private:
	const choices::choice::menu_choice_action _set_name_action;
	const menu_screen::choices::choice _choices[8];
};

} // namespace nsec::display

#endif // NSEC_DISPLAY_MAIN_MENU_CHOICES_HPP
