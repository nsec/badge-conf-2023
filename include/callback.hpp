/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2023 Jérémie Galarneau <jeremie.galarneau@gmail.com>
 */

#ifndef NSEC_CALLBACK_HPP
#define NSEC_CALLBACK_HPP

#include "display/screen.hpp"

namespace nsec {
class callback {
public:
	using raw_function = void (*)(void *data);

	callback(raw_function function, void *data) noexcept :
		_function{ function }, _user_data{ data }
	{
	}

	void operator()() const
	{
		_function(_user_data);
	}

private:
	raw_function _function;
	void *_user_data;
};
} // namespace nsec

#endif // NSEC_CALLBACK_HPP
