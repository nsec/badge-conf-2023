/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2023 Jérémie Galarneau <jeremie.galarneau@gmail.com>
 */

#include "gimp_logo.h"

#include <iostream>

int main(int argc [[maybe_unused]], const char **argv [[maybe_unused]])
{
	std::cout << "/*" << std::endl;
	std::cout << " * SPDX-License-Identifier: MIT" << std::endl;
	std::cout << " *" << std::endl;
	std::cout << " * Copyright 2023 Jérémie Galarneau <jeremie.galarneau@gmail.com>"
		  << std::endl;
	std::cout << " * " << std::endl;
	std::cout << " * This header was auto-generated; see the logo folder." << std::endl;
	std::cout << " */" << std::endl << std::endl;

	std::cout << "#include <Arduino.h>" << std::endl;

	std::cout << std::endl << "const uint8_t PROGMEM nsec_logo_data[] = { " << std::endl;

	unsigned int bit = 0;
	for (const auto pixel : header_data) {
		if (bit == 0) {
			std::cout << "0b";
		}

		std::cout << (pixel ? "1" : "0");
		if (bit++ == 7) {
			std::cout << ", ";
			bit = 0;
		}
	}

	std::cout << "};" << std::endl;
	return 0;
}