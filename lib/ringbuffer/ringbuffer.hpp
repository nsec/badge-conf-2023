// SPDX-FileCopyrightText: 2023 NorthSec
//
// SPDX-License-Identifier: MIT

#ifndef NSEC_STORAGE_BUFFER_HPP
#define NSEC_STORAGE_BUFFER_HPP

#include <EEPROM.h>
#include <stdlib.h>

#define RB_TOTAL_BYTES	  1024
#define RB_RESERVED_BYTES 4

/*
 * Use the first 32bits of the EEPROM to keep the state of the ringbuffer.
 */
#define RB_STATUS_ADDR 0 // Status flag
#define RB_COUNT_ADDR  1 // Item counter
#define RB_HEAD_ADDR   2 // Position of the write head
#define RB_UNUSED_ADDR 3 // Unused byte
#define RB_BEGIN_ADDR  4

namespace nsec::storage {
template <uint16_t BaseOffset>
class buffer {
public:
	buffer() : _capacity{ (EEPROM.length() - BaseOffset) / sizeof(uint32_t) }
	{
		if (get_status() != RB_STATUS_CLEAN) {
			clear();
		}
	}

	void clear()
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
		for (auto i = 0U; i < _capacity; i++) {
			set(i, RB_STATUS_UNINITIALIZED);
		};

		set_status(RB_STATUS_CLEAN);
	}

	bool insert(uint32_t item)
	{
		/*
		 * Don't insert duplicate items.
		 */
		if (contains(item)) {
			return false;
		}

		set_status(RB_STATUS_DIRTY);

		set(get_head(), item);

		inc_count();
		inc_head();

		set_status(RB_STATUS_CLEAN);
		return true;
	}

	bool contains(uint32_t item) const
	{
		const auto count = get_count();
		uint32_t item_i;

		for (auto i = 0U; i < count; i++) {
			EEPROM.get(item_addr(i), item_i);

			if (item == item_i) {
				return true;
			}
		}

		return false;
	}

	uint16_t count() const
	{
		return get_count();
	}

private:
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
	 * Status flag, all EEPROM cells are set to 255 for a new chip.
	 */
	uint8_t get_status() const
	{
		return EEPROM.read(RB_STATUS_ADDR + BaseOffset);
	}

	void set_status(uint8_t status)
	{
		EEPROM.update(RB_STATUS_ADDR + BaseOffset, status);
	}

	/*
	 * The count of 32bits / 4 bytes items in the ring buffer, fits in a uint8_t.
	 */
	uint16_t get_count() const
	{
		return EEPROM.read(RB_COUNT_ADDR + BaseOffset);
	}

	void set_count(uint16_t count)
	{
		EEPROM.update(RB_COUNT_ADDR + BaseOffset, count);
	}

	uint16_t inc_count()
	{
		auto count = get_count();

		// Count maxes at RB_ITEM_NB
		if (count < _capacity) {
			set_count(++count);
		}

		return count;
	}

	/*
	 * The position of the next item to be written to the ringbuffer, wraps around
	 * at 255.
	 */
	uint16_t get_head() const
	{
		return EEPROM.read(RB_HEAD_ADDR + BaseOffset);
	}

	void set_head(uint16_t head)
	{
		EEPROM.update(RB_HEAD_ADDR + BaseOffset, head);
	}

	uint16_t inc_head()
	{
		auto head = get_head();

		set_head(++head);

		return head;
	}

	// Get the eeprom address of an item.
	int item_addr(uint16_t index) const
	{
		return (index * sizeof(uint32_t)) + RB_RESERVED_BYTES + BaseOffset;
	}

	void set(uint16_t index, uint32_t item)
	{
		EEPROM.put(item_addr(index), item);
	}

	// Count of 32-bit items in the buffer.
	const uint16_t _capacity;
};
} // namespace nsec::storage

#endif // NSEC_RUNTIME_BADGE_HPP