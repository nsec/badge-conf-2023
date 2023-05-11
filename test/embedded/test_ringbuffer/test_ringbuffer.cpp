// SPDX-FileCopyrightText: 2023 NorthSec
//
// SPDX-License-Identifier: MIT

#include "ringbuffer.hpp"

#include <Arduino.h>
#include <unity.h>

void setUp()
{
	TEST_MESSAGE("Clearing the ringbuffer");

	nsec::storage::buffer<0> buffer;
	buffer.clear();
}

void test_init()
{
	const uint32_t value1 = 12345;
	const uint32_t value2 = 54321;

	nsec::storage::buffer<0> buffer;

	TEST_ASSERT_EQUAL_MESSAGE(0, buffer.count(), "init: count is 0");

	buffer.insert(value1);

	TEST_ASSERT_EQUAL_MESSAGE(1, buffer.count(), "init: count is 1");
	TEST_ASSERT_EQUAL_MESSAGE(true, buffer.contains(value1), "init: contains value");

	buffer.insert(value1);

	TEST_ASSERT_EQUAL_MESSAGE(1, buffer.count(), "init: count is still 1");
	TEST_ASSERT_EQUAL_MESSAGE(true, buffer.contains(value1), "init: contains value");

	buffer.insert(value2);

	TEST_ASSERT_EQUAL_MESSAGE(2, buffer.count(), "init: count is 2");
	TEST_ASSERT_EQUAL_MESSAGE(true, buffer.contains(value2), "init: contains value");

	buffer.insert(value2);

	TEST_ASSERT_EQUAL_MESSAGE(2, buffer.count(), "init: count is still 2");
	TEST_ASSERT_EQUAL_MESSAGE(true, buffer.contains(value2), "init: contains value");

	buffer.clear();

	TEST_ASSERT_EQUAL_MESSAGE(0, buffer.count(), "init: count is 0");
	TEST_ASSERT_EQUAL_MESSAGE(false, buffer.contains(value1), "init: doesn't contain value");
	TEST_ASSERT_EQUAL_MESSAGE(false, buffer.contains(value2), "init: doesn't contain value");
}

void setup()
{
	// Wait ~2 seconds before the Unity test runner
	// establishes connection with a board Serial interface
	delay(2000);

	UNITY_BEGIN();

	RUN_TEST(test_init);

	UNITY_END();
}
void loop()
{
}
