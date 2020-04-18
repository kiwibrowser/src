// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/task_scheduler/task.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/location.h"
#include "base/task_scheduler/task_traits.h"
#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {
namespace internal {

// Verify that the shutdown behavior of a BLOCK_SHUTDOWN delayed task is
// adjusted to SKIP_ON_SHUTDOWN. The shutown behavior of other delayed tasks
// should not change.
TEST(TaskSchedulerTaskTest, ShutdownBehaviorChangeWithDelay) {
  Task continue_on_shutdown(FROM_HERE, DoNothing(),
                            {TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
                            TimeDelta::FromSeconds(1));
  EXPECT_EQ(TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN,
            continue_on_shutdown.traits.shutdown_behavior());

  Task skip_on_shutdown(FROM_HERE, DoNothing(),
                        {TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
                        TimeDelta::FromSeconds(1));
  EXPECT_EQ(TaskShutdownBehavior::SKIP_ON_SHUTDOWN,
            skip_on_shutdown.traits.shutdown_behavior());

  Task block_shutdown(FROM_HERE, DoNothing(),
                      {TaskShutdownBehavior::BLOCK_SHUTDOWN},
                      TimeDelta::FromSeconds(1));
  EXPECT_EQ(TaskShutdownBehavior::SKIP_ON_SHUTDOWN,
            block_shutdown.traits.shutdown_behavior());
}

// Verify that the shutdown behavior of undelayed tasks is not adjusted.
TEST(TaskSchedulerTaskTest, NoShutdownBehaviorChangeNoDelay) {
  Task continue_on_shutdown(FROM_HERE, DoNothing(),
                            {TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
                            TimeDelta());
  EXPECT_EQ(TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN,
            continue_on_shutdown.traits.shutdown_behavior());

  Task skip_on_shutdown(FROM_HERE, DoNothing(),
                        {TaskShutdownBehavior::SKIP_ON_SHUTDOWN}, TimeDelta());
  EXPECT_EQ(TaskShutdownBehavior::SKIP_ON_SHUTDOWN,
            skip_on_shutdown.traits.shutdown_behavior());

  Task block_shutdown(FROM_HERE, DoNothing(),
                      {TaskShutdownBehavior::BLOCK_SHUTDOWN}, TimeDelta());
  EXPECT_EQ(TaskShutdownBehavior::BLOCK_SHUTDOWN,
            block_shutdown.traits.shutdown_behavior());
}

}  // namespace internal
}  // namespace base
