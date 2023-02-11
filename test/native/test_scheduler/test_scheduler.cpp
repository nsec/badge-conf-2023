/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2023 Jérémie Galarneau <jeremie.galarneau@gmail.com>
 */

#include "scheduler.hpp"

#include <algorithm>
#include <array>
#include <memory>
#include <random>
#include <sstream>
#include <unity.h>
#include <vector>

namespace once_scheduling {

class task_once : public nsec::scheduling::task {
public:
	explicit task_once(bool& value_to_set) : _task_was_scheduled{ value_to_set }
	{
	}

	void run([[maybe_unused]] nsec::scheduling::absolute_time_ms current_time) noexcept override
	{
		// Indicate that task ran.
		_task_was_scheduled = true;
	}

private:
	bool& _task_was_scheduled;
};

void test_task_not_ran_immediately()
{
	nsec::scheduling::scheduler<16> scheduler;
	bool task_ran = false;

	scheduler.tick(1);
	task_once my_task(task_ran);

	// The task is scheduled to run on the next tick (ideally started right after this tick
	// completes).
	scheduler.schedule_task(my_task);
	TEST_ASSERT_EQUAL_MESSAGE(
		false, task_ran, "Task scheduled \"now\" didn't run during scheduling");

	// Next tick occurs "immediately".
	scheduler.tick(1);
	TEST_ASSERT_EQUAL_MESSAGE(true, task_ran, "Task scheduled \"now\" ran at the next tick");
}

void test_task_not_ran_directly_when_scheduling()
{
	nsec::scheduling::scheduler<16> scheduler;
	bool task_ran = false;

	scheduler.tick(1);
	task_once my_task(task_ran);

	// The task should only run in 100ms, so for tick >= 101.
	scheduler.schedule_task(my_task, 100);
	TEST_ASSERT_EQUAL_MESSAGE(
		false, task_ran, "Task scheduled @ 101 not ran right after scheduling");
}

void test_task_not_ran_before_deadline()
{
	nsec::scheduling::scheduler<16> scheduler;
	bool task_ran = false;

	scheduler.tick(1);
	task_once my_task(task_ran);

	// The task should only run in 100ms, so for ticks >= 101.
	scheduler.schedule_task(my_task, 100);
	scheduler.tick(10);
	TEST_ASSERT_EQUAL_MESSAGE(false, task_ran, "Task scheduled @ 101 not ran after tick @ 10");
}

void test_task_ran_on_deadline()
{
	nsec::scheduling::scheduler<16> scheduler;
	bool task_ran = false;

	scheduler.tick(1);
	task_once my_task(task_ran);

	// The task should only run in 100ms, so for ticks >= 101.
	scheduler.schedule_task(my_task, 100);
	scheduler.tick(101);
	TEST_ASSERT_EQUAL_MESSAGE(true, task_ran, "Task scheduled @ 101 ran after tick @ 101");
}

void test_task_ran_on_late_tick()
{
	nsec::scheduling::scheduler<16> scheduler;
	bool task_ran = false;

	scheduler.tick(1);
	task_once my_task(task_ran);

	// The task should only run in 100ms, so for ticks >= 101.
	scheduler.schedule_task(my_task, 100);
	scheduler.tick(200);
	TEST_ASSERT_EQUAL_MESSAGE(true, task_ran, "Task scheduled @ 101 ran after tick @ 200");
}

void test_task_not_ran_twice()
{
	nsec::scheduling::scheduler<16> scheduler;
	bool task_ran = false;

	scheduler.tick(1);
	task_once my_task(task_ran);

	// The task should only run in 100ms, so for ticks >= 101.
	scheduler.schedule_task(my_task, 100);
	scheduler.tick(200);
	TEST_ASSERT(task_ran);

	// Reset "ran" state to validate that a task that was scheduled to run only once
	// is not ran twice.
	task_ran = false;
	scheduler.tick(500);
	TEST_ASSERT_EQUAL_MESSAGE(
		false, task_ran, "Task scheduled @ 101, and already ran, does not run twice");
}

void test_tasks_all_ran_after_deadline()
{
	nsec::scheduling::scheduler<16> scheduler;
	bool task_50_ran = false, task_100_ran = false, task_150_ran = false;

	task_once task_50(task_50_ran);
	task_once task_100(task_100_ran);
	task_once task_150(task_150_ran);

	// The task should only run in 100ms, so for ticks >= 101.
	scheduler.schedule_task(task_150, 150);
	scheduler.schedule_task(task_50, 50);
	scheduler.schedule_task(task_100, 100);
	scheduler.tick(200);

	TEST_ASSERT_EQUAL_MESSAGE(true, task_50_ran, "Task scheduled @ 50 ran after tick @ 200");
	TEST_ASSERT_EQUAL_MESSAGE(true, task_100_ran, "Task scheduled @ 100 ran after tick @ 200");
	TEST_ASSERT_EQUAL_MESSAGE(true, task_150_ran, "Task scheduled @ 150 ran after tick @ 200");
}

void test_tasks_some_ran_after_tick()
{
	nsec::scheduling::scheduler<16> scheduler;
	bool task_50_ran = false, task_100_ran = false, task_150_ran = false;

	task_once task_50(task_50_ran);
	task_once task_100(task_100_ran);
	task_once task_150(task_150_ran);

	// The task should only run in 100ms, so for ticks >= 101.
	scheduler.schedule_task(task_150, 150);
	scheduler.schedule_task(task_50, 50);
	scheduler.schedule_task(task_100, 100);
	scheduler.tick(120);

	TEST_ASSERT_EQUAL_MESSAGE(true, task_50_ran, "Task scheduled @ 50 ran after tick @ 120");
	TEST_ASSERT_EQUAL_MESSAGE(true, task_100_ran, "Task scheduled @ 100 ran after tick @ 120");
	TEST_ASSERT_EQUAL_MESSAGE(
		false, task_150_ran, "Task scheduled @ 150 didn't run after tick @ 120");
}

void test_lots_of_tasks_ran_in_order()
{
	nsec::scheduling::scheduler<16> scheduler;
	std::array<bool, 16> tasks_ran = { false };
	std::vector<std::pair<std::unique_ptr<task_once>, nsec::scheduling::relative_time_ms>> tasks;

	// Create tasks to be scheduled at ticks 5, 15, 25, 35, etc.
	for (auto i = 0U; i < tasks_ran.size(); i++) {
		tasks.emplace_back(std::make_unique<task_once>(tasks_ran[i]), (i * 10) + 5);
	}

	// Shuffle tasks to insert them in a random order in the scheduler's queue.
	// The tasks_ran array remains in order of scheduled tasks to validate the order
	// of the execution of the tasks.
	std::shuffle(std::begin(tasks), std::end(tasks), std::default_random_engine{});

	for (const auto& task_pair : tasks) {
		scheduler.schedule_task(*task_pair.first, task_pair.second);
	}

	for (auto i = 0U; i < tasks_ran.size() + 1; i++) {
		const auto current_tick = i * 10;

		scheduler.tick(current_tick);

		auto consecutive_tasks_ran = 0U;
		for (const auto& ran : tasks_ran) {
			if (ran) {
				consecutive_tasks_ran++;
			} else {
				break;
			}
		}

		std::stringstream ss;
		ss << i << " first tasks ran as of tick " << current_tick;
		TEST_ASSERT_EQUAL_MESSAGE(i, consecutive_tasks_ran, ss.str().c_str());
	}
}

} // namespace once_scheduling

namespace periodic_scheduling {

class periodic_task : public nsec::scheduling::periodic_task {
public:
	periodic_task(nsec::scheduling::relative_time_ms period_ms,
		      unsigned int& value_to_increment) :
		nsec::scheduling::periodic_task(period_ms), _value_to_increment{ value_to_increment }
	{
	}

	void run([[maybe_unused]] nsec::scheduling::absolute_time_ms current_time) noexcept override
	{
		// Indicate that task ran.
		_value_to_increment++;
	}

private:
	unsigned int& _value_to_increment;
};

class periodic_task_die_after_3 : public nsec::scheduling::periodic_task {
public:
	periodic_task_die_after_3(nsec::scheduling::relative_time_ms period_ms,
				  unsigned int& value_to_increment) :
		nsec::scheduling::periodic_task(period_ms), _value_to_increment{ value_to_increment }
	{
	}

	void run([[maybe_unused]] nsec::scheduling::absolute_time_ms current_time) noexcept override
	{
		// Indicate that task ran.
		_value_to_increment++;
		if (_value_to_increment == 3) {
			// This task should no longer run.
			kill();
		}
	}

private:
	unsigned int& _value_to_increment;
};

void test_task_not_ran_before_deadline()
{
	nsec::scheduling::scheduler<16> scheduler;
	unsigned int task_run_count = 0;

	// Run every 100 ms.
	periodic_task my_task(100, task_run_count);

	// The task should run every 100 ms, starting in 100 ms.
	scheduler.schedule_task(my_task, my_task.period_ms());
	scheduler.tick(50);
	TEST_ASSERT_EQUAL_MESSAGE(
		0, task_run_count, "Periodic task scheduled @ 100 not run with tick @ 50");
}

void test_task_ran_on_deadline()
{
	nsec::scheduling::scheduler<16> scheduler;
	unsigned int task_run_count = 0;

	// Run every 100 ms.
	periodic_task my_task(100, task_run_count);

	// The task should run every 100 ms, starting in 100 ms.
	scheduler.schedule_task(my_task, my_task.period_ms());
	scheduler.tick(100);
	TEST_ASSERT_EQUAL_MESSAGE(
		1, task_run_count, "Periodic task scheduled @ 100 ran during tick @ 100");
}

void test_task_second_run_not_before_deadline()
{
	nsec::scheduling::scheduler<16> scheduler;
	unsigned int task_run_count = 0;

	// Run every 100 ms.
	periodic_task my_task(100, task_run_count);

	// The task should run every 100 ms, starting in 100 ms.
	scheduler.schedule_task(my_task, my_task.period_ms());
	scheduler.tick(120);
	TEST_ASSERT_EQUAL_MESSAGE(
		1, task_run_count, "Periodic task scheduled @ 100 ran during tick @ 120");

	scheduler.tick(150);
	TEST_ASSERT_EQUAL_MESSAGE(1,
				  task_run_count,
				  "Periodic task scheduled @ 200 didn't run twice with tick @ 150");
}

void test_task_rescheduled()
{
	nsec::scheduling::scheduler<16> scheduler;
	unsigned int task_run_count = 0;

	// Run every 100 ms.
	periodic_task my_task(100, task_run_count);

	// The task should run every 100 ms, starting in 100 ms.
	scheduler.schedule_task(my_task, my_task.period_ms());
	scheduler.tick(100);
	TEST_ASSERT_EQUAL_MESSAGE(
		1, task_run_count, "Periodic task scheduled @ 100 ran during tick @ 100");
	scheduler.tick(200);
	TEST_ASSERT_EQUAL_MESSAGE(
		2, task_run_count, "Periodic task scheduled @ 200 ran during tick @ 200");
	scheduler.tick(300);
	TEST_ASSERT_EQUAL_MESSAGE(
		3, task_run_count, "Periodic task scheduled @ 300 ran during tick @ 300");
}

void test_task_die()
{
	nsec::scheduling::scheduler<16> scheduler;
	unsigned int task_run_count = 0;

	// Run every 100 ms.
	periodic_task_die_after_3 my_task(100, task_run_count);

	// The task should run every 100 ms, starting in 100 ms.
	scheduler.schedule_task(my_task, my_task.period_ms());
	scheduler.tick(100);
	TEST_ASSERT_EQUAL_MESSAGE(
		1, task_run_count, "Periodic task scheduled @ 100 ran during tick @ 100");
	scheduler.tick(200);
	TEST_ASSERT_EQUAL_MESSAGE(
		2, task_run_count, "Periodic task scheduled @ 200 ran during tick @ 200");
	scheduler.tick(300);
	TEST_ASSERT_EQUAL_MESSAGE(
		3, task_run_count, "Periodic task scheduled @ 300 ran during tick @ 300");

	scheduler.tick(400);
	TEST_ASSERT_EQUAL_MESSAGE(
		3,
		task_run_count,
		"Periodic task scheduled to run only three times only ran three times");
}

} // namespace periodic_scheduling

int main([[maybe_unused]] int argc, [[maybe_unused]] char **argv)
{
	UNITY_BEGIN();

	RUN_TEST(once_scheduling::test_task_not_ran_immediately);
	RUN_TEST(once_scheduling::test_task_not_ran_before_deadline);
	RUN_TEST(once_scheduling::test_task_not_ran_directly_when_scheduling);
	RUN_TEST(once_scheduling::test_task_not_ran_twice);
	RUN_TEST(once_scheduling::test_task_ran_on_deadline);
	RUN_TEST(once_scheduling::test_task_ran_on_late_tick);
	RUN_TEST(once_scheduling::test_tasks_all_ran_after_deadline);
	RUN_TEST(once_scheduling::test_tasks_some_ran_after_tick);
	RUN_TEST(once_scheduling::test_lots_of_tasks_ran_in_order);

	RUN_TEST(periodic_scheduling::test_task_not_ran_before_deadline);
	RUN_TEST(periodic_scheduling::test_task_ran_on_deadline);
	RUN_TEST(periodic_scheduling::test_task_rescheduled);
	RUN_TEST(periodic_scheduling::test_task_die);

	return UNITY_END();
}
