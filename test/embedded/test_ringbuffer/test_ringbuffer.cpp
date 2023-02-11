// SPDX-FileCopyrightText: 2023 NorthSec
//
// SPDX-License-Identifier: MIT

#include <unity.h>
#include "ringbuffer.hpp"

#include <Arduino.h>

void setUp() {
	TEST_MESSAGE("Clearing the ringbuffer");
	nsec::ringbuffer::clear();
}

void test_init()
{
	const uint32_t value1 = 12345;
	const uint32_t value2 = 54321;

	nsec::ringbuffer::setup();

	TEST_ASSERT_EQUAL_MESSAGE(0, nsec::ringbuffer::count(), "init: count is 0");

	nsec::ringbuffer::insert(value1);

	TEST_ASSERT_EQUAL_MESSAGE(1, nsec::ringbuffer::count(), "init: count is 1");
	TEST_ASSERT_EQUAL_MESSAGE(true, nsec::ringbuffer::contains(value1), "init: contains value");

	nsec::ringbuffer::insert(value1);

	TEST_ASSERT_EQUAL_MESSAGE(1, nsec::ringbuffer::count(), "init: count is still 1");
	TEST_ASSERT_EQUAL_MESSAGE(true, nsec::ringbuffer::contains(value1), "init: contains value");

	nsec::ringbuffer::insert(value2);

	TEST_ASSERT_EQUAL_MESSAGE(2, nsec::ringbuffer::count(), "init: count is 2");
	TEST_ASSERT_EQUAL_MESSAGE(true, nsec::ringbuffer::contains(value2), "init: contains value");

	nsec::ringbuffer::insert(value2);

	TEST_ASSERT_EQUAL_MESSAGE(2, nsec::ringbuffer::count(), "init: count is still 2");
	TEST_ASSERT_EQUAL_MESSAGE(true, nsec::ringbuffer::contains(value2), "init: contains value");

	nsec::ringbuffer::clear();

	TEST_ASSERT_EQUAL_MESSAGE(0, nsec::ringbuffer::count(), "init: count is 0");
	TEST_ASSERT_EQUAL_MESSAGE(false, nsec::ringbuffer::contains(value1), "init: doesn't contain value");
	TEST_ASSERT_EQUAL_MESSAGE(false, nsec::ringbuffer::contains(value2), "init: doesn't contain value");
}

void setup() {
	// Wait ~2 seconds before the Unity test runner
	// establishes connection with a board Serial interface
	delay(2000);

	UNITY_BEGIN();

	RUN_TEST(test_init);

	UNITY_END();
}
void loop() {}
