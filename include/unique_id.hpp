// SPDX-FileCopyrightText: 2019 Luiz Henrique Cassettari
//
// SPDX-License-Identifier: MIT
//
#ifndef NSEC_UNIQUE_ID_HPP
#define NSEC_UNIQUE_ID_HPP

#include <avr/boot.h>
#include <stdint.h>

#if defined(__AVR_ATmega328PB__)
#define UniqueIDsize 10
#else
#define UniqueIDsize 9
#endif

#define UniqueID (_UniqueID.id)

class ArduinoUniqueID {
public:
	ArduinoUniqueID();
	uint8_t id[UniqueIDsize];
};

extern ArduinoUniqueID _UniqueID;

#endif /* NSEC_UNIQUE_ID_HPP */
