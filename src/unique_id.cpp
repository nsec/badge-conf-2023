// SPDX-FileCopyrightText: 2019 Luiz Henrique Cassettari
//
// SPDX-License-Identifier: MIT

#include "unique_id.hpp"

#include <stddef.h>

ArduinoUniqueID::ArduinoUniqueID()
{
	for (size_t i = 0; i < UniqueIDsize; i++) {
#if defined(__AVR_ATmega328PB__)
		id[i] = boot_signature_byte_get(0x0E + i);
#else
		id[i] = boot_signature_byte_get(0x0E + i + (i > 5 ? 1 : 0));
#endif
	}
}

ArduinoUniqueID _UniqueID;
