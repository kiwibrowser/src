// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/common/one_shot_event.h"

#include "base/bind.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/test/test_simple_task_runner.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace extensions {

namespace {

void Increment(int* i) { ++*i; }

TEST(OneShotEventTest, RecordsSignal) {
  OneShotEvent event;
  EXPECT_FALSE(event.is_signaled());
  event.Signal();
  EXPECT_TRUE(event.is_signaled());
}

TEST(OneShotEventTest, CallsQueueAsDistinctTask) {
  OneShotEvent event;
  scoped_refptr<base::TestSimpleTaskRunner> runner(
      new base::TestSimpleTaskRunner);
  int i = 0;
  event.Post(FROM_HERE, base::Bind(&Increment, &i), runner);
  event.Post(FROM_HERE, base::Bind(&Increment, &i), runner);
  EXPECT_EQ(0U, runner->NumPendingTasks());
  event.Signal();

  auto pending_tasks = runner->TakePendingTasks();
  ASSERT_EQ(2U, pending_tasks.size());
  EXPECT_NE(pending_tasks[0].location.line_number(),
            pending_tasks[1].location.line_number())
      << "Make sure FROM_HERE is propagated.";
}

TEST(OneShotEventTest, CallsQueue) {
  OneShotEvent event;
  scoped_refptr<base::TestSimpleTaskRunner> runner(
      new base::TestSimpleTaskRunner);
  int i = 0;
  event.Post(FROM_HERE, base::Bind(&Increment, &i), runner);
  event.Post(FROM_HERE, base::Bind(&Increment, &i), runner);
  EXPECT_EQ(0U, runner->NumPendingTasks());
  event.Signal();
  ASSERT_EQ(2U, runner->NumPendingTasks());

  EXPECT_EQ(0, i);
  runner->RunPendingTasks();
  EXPECT_EQ(2, i);
}

TEST(OneShotEventTest, CallsAfterSignalDontRunInline) {
  OneShotEvent event;
  scoped_refptr<base::TestSimpleTaskRunner> runner(
      new base::TestSimpleTaskRunner);
  int i = 0;

  event.Signal();
  event.Post(FROM_HERE, base::Bind(&Increment, &i), runner);
  EXPECT_EQ(1U, runner->NumPendingTasks());
  EXPECT_EQ(0, i);
  runner->RunPendingTasks();
  EXPECT_EQ(1, i);
}

TEST(OneShotEventTest, PostDefaultsToCurrentMessageLoop) {
  OneShotEvent event;
  scoped_refptr<base::TestSimpleTaskRunner> runner(
      new base::TestSimpleTaskRunner);
  base::MessageLoop loop;
  int runner_i = 0;
  int loop_i = 0;

  event.Post(FROM_HERE, base::Bind(&Increment, &runner_i), runner);
  event.Post(FROM_HERE, base::Bind(&Increment, &loop_i));
  event.Signal();
  EXPECT_EQ(1U, runner->NumPendingTasks());
  EXPECT_EQ(0, runner_i);
  runner->RunPendingTasks();
  EXPECT_EQ(1, runner_i);
  EXPECT_EQ(0, loop_i);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, loop_i);
}

void CheckSignaledAndPostIncrement(
    OneShotEvent* event,
    const scoped_refptr<base::SingleThreadTaskRunner>& runner,
    int* i) {
  EXPECT_TRUE(event->is_signaled());
  event->Post(FROM_HERE, base::Bind(&Increment, i), runner);
}

TEST(OneShotEventTest, IsSignaledAndPostsFromCallbackWork) {
  OneShotEvent event;
  scoped_refptr<base::TestSimpleTaskRunner> runner(
      new base::TestSimpleTaskRunner);
  int i = 0;

  event.Post(FROM_HERE,
             base::Bind(&CheckSignaledAndPostIncrement, &event, runner, &i),
             runner);
  EXPECT_EQ(0, i);
  event.Signal();

  // CheckSignaledAndPostIncrement is queued on |runner|.
  EXPECT_EQ(1U, runner->NumPendingTasks());
  EXPECT_EQ(0, i);
  runner->RunPendingTasks();
  // Increment is queued on |runner|.
  EXPECT_EQ(1U, runner->NumPendingTasks());
  EXPECT_EQ(0, i);
  runner->RunPendingTasks();
  // Increment has run.
  EXPECT_EQ(0U, runner->NumPendingTasks());
  EXPECT_EQ(1, i);
}

}  // namespace
}  // namespace extensions
