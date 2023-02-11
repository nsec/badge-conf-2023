// SPDX-FileCopyrightText: 2023 NorthSec
//
// SPDX-License-Identifier: MIT

#include <stdint.h>

namespace nsec::ringbuffer {
void clear();
void setup();
void insert(uint32_t item);
bool contains(uint32_t item);
uint8_t count();
} // namespace nsec::ringbuffer
