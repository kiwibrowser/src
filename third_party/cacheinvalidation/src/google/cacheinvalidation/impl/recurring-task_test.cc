// Copyright 2012 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Unit tests for the RecurringTask class.

#include "google/cacheinvalidation/client_test_internal.pb.h"
#include "google/cacheinvalidation/deps/googletest.h"
#include "google/cacheinvalidation/deps/random.h"
#include "google/cacheinvalidation/deps/string_util.h"
#include "google/cacheinvalidation/impl/recurring-task.h"
#include "google/cacheinvalidation/test/deterministic-scheduler.h"
#include "google/cacheinvalidation/test/test-logger.h"
#include "google/cacheinvalidation/test/test-utils.h"

namespace invalidation {

class RecurringTaskTest;

/* A Test task that tracks how many times it has been called and returns
 * true when the number of times, it has been called is less than the expected
 * number.
 */
class TestTask : public RecurringTask {
 public:
  /* Initial delay used by the TestTask. */
  static TimeDelta initial_delay;

  /* Timeout delay used by the TestTask. */
  static TimeDelta timeout_delay;

  /* Creates a test task.
   *
   * |scheduler| Scheduler for the scheduling the task as needed.
   * |logger| A logger.
   * |smearer| For spreading/randomizing the delays.
   * |delay_generator| An exponential backoff generator for task retries (if
   *   any).
   * |test_name| The name of the current test.
   * |max_runs| Maximum number of runs that are allowed.
   *
   * Space for all the objects with pointers is owned by the caller.
   */
  TestTask(Scheduler* scheduler, Logger* logger, Smearer* smearer,
           ExponentialBackoffDelayGenerator* delay_generator,
           const string& test_name, int max_runs)
      : RecurringTask(test_name, scheduler, logger, smearer, delay_generator,
                      initial_delay, timeout_delay),
        current_runs(0),
        max_runs_(max_runs),
        scheduler_(scheduler),
        logger_(logger) {
  }

  virtual ~TestTask() {}

  // The actual implementation as required by the RecurringTask.
  virtual bool RunTask() {
    current_runs++;
    TLOG(logger_, INFO, "(%d) Task running: %d",
         scheduler_->GetCurrentTime().ToInternalValue(), current_runs);
    return current_runs < max_runs_;
  }

  /* The number of times that the task has been run. */
  int current_runs;

 private:
  /* Maximum number of runs that are allowed. */
  int max_runs_;

  /* Scheduler for the task. */
  Scheduler* scheduler_;

  /* A logger. */
  Logger* logger_;
};

// Tests the basic functionality of the RecurringTask abstraction.
class RecurringTaskTest : public testing::Test {
 public:
  virtual ~RecurringTaskTest() {}

  // Performs setup for RecurringTask test.
  virtual void SetUp() {
    // Initialize values that are really constants.
    initial_exp_backoff_delay = TimeDelta::FromMilliseconds(100);
    TestTask::initial_delay = TimeDelta::FromMilliseconds(10);
    TestTask::timeout_delay = TimeDelta::FromMilliseconds(50);
    end_of_test_delay = 1000 * TestTask::timeout_delay;

    // Initialize state for every test.
    random.reset(new FakeRandom(0.99));  // The test expects a value close to 1.
    logger.reset(new TestLogger());
    scheduler.reset(new SimpleDeterministicScheduler(logger.get()));
    smearer.reset(new Smearer(random.get(), 0));
    delay_generator = new ExponentialBackoffDelayGenerator(
            random.get(), initial_exp_backoff_delay, kMaxExpBackoffFactor);
    scheduler->StartScheduler();
  }

  /* Maximum delay factory used by the ExponentialBackoffDelayGenerator. */
  static const int kMaxExpBackoffFactor;

  /* Default number of runs that runTask is called in TestTask. */
  static const int kDefaultNumRuns;

  /* Initial delay used by the ExponentialBackoffDelayGenerator. */
  static TimeDelta initial_exp_backoff_delay;

  /* A long time delay that the scheduler is run for at the end of the test. */
  static TimeDelta end_of_test_delay;

  //
  // Test state maintained for every test.
  //

  // A logger.
  scoped_ptr<Logger> logger;

  // Deterministic scheduler for careful scheduling of the tasks.
  scoped_ptr<DeterministicScheduler> scheduler;

  // For randomizing delays.
  scoped_ptr<Smearer> smearer;

  // A random number generator that always generates 1.
  scoped_ptr<Random> random;

  // A delay generator (if used in the test). Not a scoped_ptr since it ends
  // up being owned by the RecurringTask.
  ExponentialBackoffDelayGenerator* delay_generator;
};

// Definitions for the static variables.
TimeDelta TestTask::initial_delay;
TimeDelta TestTask::timeout_delay;
TimeDelta RecurringTaskTest::initial_exp_backoff_delay;
TimeDelta RecurringTaskTest::end_of_test_delay;
const int RecurringTaskTest::kMaxExpBackoffFactor = 10;
const int RecurringTaskTest::kDefaultNumRuns = 8;

/* Tests a task that is run periodically at regular intervals (not exponential
 * backoff).
 */
TEST_F(RecurringTaskTest, PeriodicTask) {
  /* Create a periodic task and pass time - make sure that the task runs exactly
   * the number of times as expected.
   */
  TestTask task(scheduler.get(), logger.get(), smearer.get(), NULL,
                "PeriodicTask", kDefaultNumRuns);
  task.EnsureScheduled("testPeriodicTask");

  TimeDelta delay = TestTask::initial_delay + TestTask::timeout_delay;

  // Pass time so that the task is run kDefaultNumRuns times.
  // First time, the task is scheduled after initial_delay. Then for
  // numRuns - 1, it is scheduled after a delay of
  // initial_delay + timeout_delay.
  scheduler->PassTime(TestTask::initial_delay +
                      ((kDefaultNumRuns - 1) * delay));
  ASSERT_EQ(kDefaultNumRuns, task.current_runs);

  // Check that the passage of more time does not cause any more runs.
  scheduler->PassTime(end_of_test_delay);
  ASSERT_EQ(kDefaultNumRuns, task.current_runs);
  delete delay_generator;
}

/* Tests a task that is run periodically at regular intervals with
 * exponential backoff.
 */
TEST_F(RecurringTaskTest, ExponentialBackoffTask) {
  /* Create a periodic task and pass time - make sure that the task runs
   * exactly the number of times as expected.
   */
  TestTask task(scheduler.get(), logger.get(), smearer.get(),
                delay_generator, "ExponentialBackoffTask", kDefaultNumRuns);
  task.EnsureScheduled("testExponentialBackoffTask");

  // Pass enough time so that exactly one event runs, two events run etc.
  scheduler->PassTime(TestTask::initial_delay);
  ASSERT_EQ(1, task.current_runs);
  scheduler->PassTime(TestTask::timeout_delay + initial_exp_backoff_delay);
  ASSERT_EQ(2, task.current_runs);
  scheduler->PassTime(
      TestTask::timeout_delay + (2 * initial_exp_backoff_delay));
  ASSERT_EQ(3, task.current_runs);
  scheduler->PassTime(
      TestTask::timeout_delay + (4 * initial_exp_backoff_delay));
  ASSERT_EQ(4, task.current_runs);

  // Check that the passage of more time does not cause any more runs.
  scheduler->PassTime(end_of_test_delay);
  ASSERT_EQ(kDefaultNumRuns, task.current_runs);
}

/* Tests a one-shot task (i.e. no repetition) that is run twice. */
TEST_F(RecurringTaskTest, OneShotTask) {
  /* Create a no-repeating task and pass time - make sure that the task runs
   * exactly once. Run it again - and make sure it is run again.
   */

  // Call ensureScheduled multiple times; ensure that the event is not scheduled
  // multiple times.
  TestTask task(scheduler.get(), logger.get(), smearer.get(),
                delay_generator, "OneShotTask", 1);
  task.EnsureScheduled("testOneShotTask");
  task.EnsureScheduled("testOneShotTask-2");
  task.EnsureScheduled("testOneShotTask-3");

  // Pass enough time so that exactly one event runs.
  scheduler->PassTime(TestTask::initial_delay);
  ASSERT_EQ(1, task.current_runs);

  // Pass enough time so that exactly another event runs.
  task.EnsureScheduled("testOneShotTask-4");
  scheduler->PassTime(TestTask::initial_delay);
  ASSERT_EQ(2, task.current_runs);

  // Check that the passage of more time does not cause any more runs.
  scheduler->PassTime(end_of_test_delay);
  ASSERT_EQ(2, task.current_runs);
}

}  // namespace invalidation
