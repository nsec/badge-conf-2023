// SPDX-FileCopyrightText: 2023 NorthSec
//
// SPDX-License-Identifier: MIT

#include "ringbuffer.hpp"

#include <EEPROM.h>
#include <stdio.h>
#include <stdlib.h>

#define RB_TOTAL_BYTES	  1024
#define RB_RESERVED_BYTES 4

/*
 * Number of 32bit items in the 'ringbuffer'.
 */
#define RB_ITEM_NB ((RB_TOTAL_BYTES - RB_RESERVED_BYTES) / sizeof(uint32_t))

/*
 * Use the first 32bits of the EEPROM to keep the state of the ringbuffer.
 */
#define RB_STATUS_ADDR 0 // Status flag
#define RB_COUNT_ADDR  1 // Item counter
#define RB_HEAD_ADDR   2 // Position of the write head
#define RB_UNUSED_ADDR 3 // Unused byte
#define RB_BEGIN_ADDR  4

/*
 * Each byte in a fresh chip will be set to 255, use this value to identify the
 * unitialized state.
 */
enum RB_STATUS : uint8_t {
	RB_STATUS_CLEAN = 0,
	RB_STATUS_DIRTY = 1,
	RB_STATUS_UNINITIALIZED = 255,
};

/*
 * Make sure the EEPROM is the size we expect.
 */
static void check_eeprom_length()
{
	if (EEPROM.length() != RB_TOTAL_BYTES) {
		printf("ERROR: wrong EEPROM size (%u != %u)\n", EEPROM.length(), RB_TOTAL_BYTES);
		abort();
	}
}

/*
 * Status flag, all EEPROM cells are set to 255 for a new chip.
 */
static uint8_t get_status()
{
	return EEPROM.read(RB_STATUS_ADDR);
}

static void set_status(uint8_t status)
{
	EEPROM.update(RB_STATUS_ADDR, status);
}

/*
 * The count of 32bits / 4 bytes items in the ring buffer, fits in a uint8_t.
 */

static uint8_t get_count()
{
	return EEPROM.read(RB_COUNT_ADDR);
}

static void set_count(uint8_t count)
{
	EEPROM.update(RB_COUNT_ADDR, count);
}

static uint8_t inc_count()
{
	uint8_t count = get_count();

	// Count maxes at RB_ITEM_NB
	if (count < RB_ITEM_NB) {
		set_count(++count);
	}

	return count;
}

/*
 * The position of the next item to be written to the ringbuffer, wraps around
 * at 255.
 */

static uint8_t get_head()
{
	return EEPROM.read(RB_HEAD_ADDR);
}

static void set_head(uint8_t head)
{
	EEPROM.update(RB_HEAD_ADDR, head);
}

static uint8_t inc_head()
{
	uint8_t head = get_head();

	set_head(++head);

	return head;
}

/*
 * Get the eeprom address of an item.
 */
static int item_addr(uint8_t index)
{
	return (index * sizeof(uint32_t)) + RB_RESERVED_BYTES;
}

static void set(uint8_t index, uint32_t item)
{
	EEPROM.put(item_addr(index), item);
}

/*
 * Reset the ring buffer.
 */
void nsec::ringbuffer::clear()
{
	/*
	 * Make sure the status is set to uninitialized, if we are interrupted
	 * this will ensure the reset is restarted.
	 */
	set_status(RB_STATUS_UNINITIALIZED);

	set_count(0);
	set_head(0);

	/*
	 * Clear all cells.
	 */
	for (uint8_t i = 0; i < (uint8_t) RB_ITEM_NB; i++) {
		set(i, RB_STATUS_UNINITIALIZED);
	};

	set_status(RB_STATUS_CLEAN);
}

/*
 * Initial setup.
 */
void nsec::ringbuffer::setup()
{
	check_eeprom_length();

	if (get_status() != RB_STATUS_CLEAN) {
		clear();
	}
}

void nsec::ringbuffer::insert(uint32_t item)
{
	/*
	 * Don't insert duplicate items.
	 */
	if (contains(item)) {
		return;
	}

	set_status(RB_STATUS_DIRTY);

	set(get_head(), item);

	inc_count();
	inc_head();

	set_status(RB_STATUS_CLEAN);
}

bool nsec::ringbuffer::contains(uint32_t item)
{
	const uint8_t count = get_count();
	uint32_t item_i;

	for (uint8_t i = 0; i < count; i++) {
		EEPROM.get(item_addr(i), item_i);

		if (item == item_i) {
			return true;
		}
	}

	return false;
}

uint8_t nsec::ringbuffer::count()
{
	return get_count();
}
