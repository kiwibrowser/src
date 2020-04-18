// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/fuchsia/async_dispatcher.h"

#include <lib/async/default.h>
#include <lib/async/task.h>
#include <lib/async/wait.h>

#include "base/callback.h"
#include "base/fuchsia/scoped_zx_handle.h"
#include "base/test/test_timeouts.h"
#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {

namespace {

struct TestTask : public async_task_t {
  explicit TestTask() {
    state = ASYNC_STATE_INIT;
    handler = &TaskProc;
    deadline = 0;
  }

  static void TaskProc(async_t* async, async_task_t* task, zx_status_t status);

  int num_calls = 0;
  int repeats = 1;
  OnceClosure on_call;
  zx_status_t last_status = ZX_OK;
};

// static
void TestTask::TaskProc(async_t* async,
                        async_task_t* task,
                        zx_status_t status) {
  EXPECT_EQ(async, async_get_default());
  EXPECT_TRUE(status == ZX_OK || status == ZX_ERR_CANCELED)
      << "status: " << status;

  auto* test_task = static_cast<TestTask*>(task);
  test_task->num_calls++;
  test_task->last_status = status;

  if (!test_task->on_call.is_null())
    std::move(test_task->on_call).Run();

  if (test_task->num_calls < test_task->repeats)
    async_post_task(async, task);
};

struct TestWait : public async_wait_t {
  TestWait(zx_handle_t handle,
           zx_signals_t signals) {
    state = ASYNC_STATE_INIT;
    handler = &HandleProc;
    object = handle;
    trigger = signals;
  }

  static void HandleProc(async_t* async,
                         async_wait_t* wait,
                         zx_status_t status,
                         const zx_packet_signal_t* signal);
  int num_calls = 0;
  OnceClosure on_call;
  zx_status_t last_status = ZX_OK;
};

// static
void TestWait::HandleProc(async_t* async,
                          async_wait_t* wait,
                          zx_status_t status,
                          const zx_packet_signal_t* signal) {
  EXPECT_EQ(async, async_get_default());
  EXPECT_TRUE(status == ZX_OK || status == ZX_ERR_CANCELED)
      << "status: " << status;

  auto* test_wait = static_cast<TestWait*>(wait);

  test_wait->num_calls++;
  test_wait->last_status = status;

  if (!test_wait->on_call.is_null())
    std::move(test_wait->on_call).Run();
}

}  // namespace

class AsyncDispatcherTest : public testing::Test {
 public:
  AsyncDispatcherTest() {
    dispatcher_ = std::make_unique<AsyncDispatcher>();

    async_ = async_get_default();
    EXPECT_TRUE(async_);

    EXPECT_EQ(zx_socket_create(ZX_SOCKET_DATAGRAM, socket1_.receive(),
                               socket2_.receive()),
              ZX_OK);
  }

  ~AsyncDispatcherTest() override = default;

  void RunUntilIdle() {
    while (true) {
      zx_status_t status = dispatcher_->DispatchOrWaitUntil(0);
      if (status != ZX_OK) {
        EXPECT_EQ(status, ZX_ERR_TIMED_OUT);
        break;
      }
    }
  }

 protected:
  std::unique_ptr<AsyncDispatcher> dispatcher_;

  async_t* async_ = nullptr;

  base::ScopedZxHandle socket1_;
  base::ScopedZxHandle socket2_;
};

TEST_F(AsyncDispatcherTest, PostTask) {
  TestTask task;
  ASSERT_EQ(async_post_task(async_, &task), ZX_OK);
  dispatcher_->DispatchOrWaitUntil(0);
  EXPECT_EQ(task.num_calls, 1);
  EXPECT_EQ(task.last_status, ZX_OK);
}

TEST_F(AsyncDispatcherTest, TaskRepeat) {
  TestTask task;
  task.repeats = 2;
  ASSERT_EQ(async_post_task(async_, &task), ZX_OK);
  RunUntilIdle();
  EXPECT_EQ(task.num_calls, 2);
  EXPECT_EQ(task.last_status, ZX_OK);
}

TEST_F(AsyncDispatcherTest, DelayedTask) {
  TestTask task;
  constexpr auto kDelay = TimeDelta::FromMilliseconds(5);
  TimeTicks started = TimeTicks::Now();
  task.deadline = zx_deadline_after(kDelay.InNanoseconds());
  ASSERT_EQ(async_post_task(async_, &task), ZX_OK);
  zx_status_t status = dispatcher_->DispatchOrWaitUntil(zx_deadline_after(
      (kDelay + TestTimeouts::tiny_timeout()).InNanoseconds()));
  EXPECT_EQ(status, ZX_OK);
  EXPECT_GE(TimeTicks::Now() - started, kDelay);
}

TEST_F(AsyncDispatcherTest, CancelTask) {
  TestTask task;
  ASSERT_EQ(async_post_task(async_, &task), ZX_OK);
  ASSERT_EQ(async_cancel_task(async_, &task), ZX_OK);
  RunUntilIdle();
  EXPECT_EQ(task.num_calls, 0);
}

TEST_F(AsyncDispatcherTest, TaskObserveShutdown) {
  TestTask task;
  ASSERT_EQ(async_post_task(async_, &task), ZX_OK);
  dispatcher_.reset();

  EXPECT_EQ(task.num_calls, 1);
  EXPECT_EQ(task.last_status, ZX_ERR_CANCELED);
}

TEST_F(AsyncDispatcherTest, Wait) {
  TestWait wait(socket1_.get(), ZX_SOCKET_READABLE);
  EXPECT_EQ(async_begin_wait(async_, &wait), ZX_OK);

  // Handler shouldn't be called because the event wasn't signaled.
  RunUntilIdle();
  EXPECT_EQ(wait.num_calls, 0);

  char byte = 0;
  EXPECT_EQ(zx_socket_write(socket2_.get(), /*options=*/0, &byte, sizeof(byte),
                            /*actual=*/nullptr),
            ZX_OK);

  zx_status_t status = dispatcher_->DispatchOrWaitUntil(
      zx_deadline_after(TestTimeouts::tiny_timeout().InNanoseconds()));
  EXPECT_EQ(status, ZX_OK);

  EXPECT_EQ(wait.num_calls, 1);
  EXPECT_EQ(wait.last_status, ZX_OK);
}

TEST_F(AsyncDispatcherTest, CancelWait) {
  TestWait wait(socket1_.get(), ZX_SOCKET_READABLE);
  EXPECT_EQ(async_begin_wait(async_, &wait), ZX_OK);

  char byte = 0;
  EXPECT_EQ(zx_socket_write(socket2_.get(), /*options=*/0, &byte, sizeof(byte),
                            /*actual=*/nullptr),
            ZX_OK);

  EXPECT_EQ(async_cancel_wait(async_, &wait), ZX_OK);

  RunUntilIdle();
  EXPECT_EQ(wait.num_calls, 0);
}

TEST_F(AsyncDispatcherTest, WaitShutdown) {
  TestWait wait(socket1_.get(), ZX_SOCKET_READABLE);
  EXPECT_EQ(async_begin_wait(async_, &wait), ZX_OK);
  RunUntilIdle();
  dispatcher_.reset();

  EXPECT_EQ(wait.num_calls, 1);
  EXPECT_EQ(wait.last_status, ZX_ERR_CANCELED);
}

}  // namespace base
