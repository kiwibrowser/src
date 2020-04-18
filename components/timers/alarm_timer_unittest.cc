// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <sys/timerfd.h>

#include <memory>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/files/file_descriptor_watcher_posix.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/platform_thread.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "components/timers/alarm_timer_chromeos.h"
#include "testing/gtest/include/gtest/gtest.h"

// Most of these tests have been lifted right out of timer_unittest.cc with only
// cosmetic changes. We want the AlarmTimer to be a drop-in replacement for the
// regular Timer so it should pass the same tests as the Timer class.
namespace timers {
namespace {
const base::TimeDelta kTenMilliseconds = base::TimeDelta::FromMilliseconds(10);

class OneShotAlarmTimerTester {
 public:
  OneShotAlarmTimerTester(bool* did_run, base::TimeDelta delay)
      : did_run_(did_run),
        delay_(delay),
        timer_(new timers::OneShotAlarmTimer()) {}
  void Start() {
    timer_->Start(FROM_HERE, delay_, base::Bind(&OneShotAlarmTimerTester::Run,
                                                base::Unretained(this)));
  }

 private:
  void Run() {
    *did_run_ = true;

    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::RunLoop::QuitCurrentWhenIdleClosureDeprecated());
  }

  bool* did_run_;
  const base::TimeDelta delay_;
  std::unique_ptr<timers::OneShotAlarmTimer> timer_;

  DISALLOW_COPY_AND_ASSIGN(OneShotAlarmTimerTester);
};

class OneShotSelfDeletingAlarmTimerTester {
 public:
  OneShotSelfDeletingAlarmTimerTester(bool* did_run, base::TimeDelta delay)
      : did_run_(did_run),
        delay_(delay),
        timer_(new timers::OneShotAlarmTimer()) {}
  void Start() {
    timer_->Start(FROM_HERE, delay_,
                  base::Bind(&OneShotSelfDeletingAlarmTimerTester::Run,
                             base::Unretained(this)));
  }

 private:
  void Run() {
    *did_run_ = true;
    timer_.reset();

    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::RunLoop::QuitCurrentWhenIdleClosureDeprecated());
  }

  bool* did_run_;
  const base::TimeDelta delay_;
  std::unique_ptr<timers::OneShotAlarmTimer> timer_;

  DISALLOW_COPY_AND_ASSIGN(OneShotSelfDeletingAlarmTimerTester);
};

class RepeatingAlarmTimerTester {
 public:
  RepeatingAlarmTimerTester(bool* did_run, base::TimeDelta delay)
      : did_run_(did_run),
        delay_(delay),
        counter_(10),
        timer_(new timers::RepeatingAlarmTimer()) {}
  void Start() {
    timer_->Start(FROM_HERE, delay_, base::Bind(&RepeatingAlarmTimerTester::Run,
                                                base::Unretained(this)));
  }

 private:
  void Run() {
    if (--counter_ == 0) {
      *did_run_ = true;
      timer_->Stop();

      base::ThreadTaskRunnerHandle::Get()->PostTask(
          FROM_HERE, base::RunLoop::QuitCurrentWhenIdleClosureDeprecated());
    }
  }

  bool* did_run_;
  const base::TimeDelta delay_;
  int counter_;
  std::unique_ptr<timers::RepeatingAlarmTimer> timer_;

  DISALLOW_COPY_AND_ASSIGN(RepeatingAlarmTimerTester);
};

}  // namespace

//-----------------------------------------------------------------------------
// Each test is run against each type of MessageLoop.  That way we are sure
// that timers work properly in all configurations.

TEST(AlarmTimerTest, OneShotAlarmTimer) {
  base::MessageLoopForIO loop;
  base::FileDescriptorWatcher file_descriptor_watcher(&loop);

  bool did_run = false;
  OneShotAlarmTimerTester f(&did_run, kTenMilliseconds);
  f.Start();

  base::RunLoop().Run();

  EXPECT_TRUE(did_run);
}

TEST(AlarmTimerTest, OneShotAlarmTimer_Cancel) {
  base::MessageLoopForIO loop;
  base::FileDescriptorWatcher file_descriptor_watcher(&loop);

  bool did_run_a = false;
  OneShotAlarmTimerTester* a =
      new OneShotAlarmTimerTester(&did_run_a, kTenMilliseconds);

  // This should run before the timer expires.
  base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE, a);

  // Now start the timer.
  a->Start();

  bool did_run_b = false;
  OneShotAlarmTimerTester b(&did_run_b, kTenMilliseconds);
  b.Start();

  base::RunLoop().Run();

  EXPECT_FALSE(did_run_a);
  EXPECT_TRUE(did_run_b);
}

// If underlying timer does not handle this properly, we will crash or fail
// in full page heap environment.
TEST(AlarmTimerTest, OneShotSelfDeletingAlarmTimer) {
  base::MessageLoopForIO loop;
  base::FileDescriptorWatcher file_descriptor_watcher(&loop);

  bool did_run = false;
  OneShotSelfDeletingAlarmTimerTester f(&did_run, kTenMilliseconds);
  f.Start();

  base::RunLoop().Run();

  EXPECT_TRUE(did_run);
}

TEST(AlarmTimerTest, RepeatingAlarmTimer) {
  base::MessageLoopForIO loop;
  base::FileDescriptorWatcher file_descriptor_watcher(&loop);

  bool did_run = false;
  RepeatingAlarmTimerTester f(&did_run, kTenMilliseconds);
  f.Start();

  base::RunLoop().Run();

  EXPECT_TRUE(did_run);
}

TEST(AlarmTimerTest, RepeatingAlarmTimer_Cancel) {
  base::MessageLoopForIO loop;
  base::FileDescriptorWatcher file_descriptor_watcher(&loop);

  bool did_run_a = false;
  RepeatingAlarmTimerTester* a =
      new RepeatingAlarmTimerTester(&did_run_a, kTenMilliseconds);

  // This should run before the timer expires.
  base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE, a);

  // Now start the timer.
  a->Start();

  bool did_run_b = false;
  RepeatingAlarmTimerTester b(&did_run_b, kTenMilliseconds);
  b.Start();

  base::RunLoop().Run();

  EXPECT_FALSE(did_run_a);
  EXPECT_TRUE(did_run_b);
}

TEST(AlarmTimerTest, RepeatingAlarmTimerZeroDelay) {
  base::MessageLoopForIO loop;
  base::FileDescriptorWatcher file_descriptor_watcher(&loop);

  bool did_run = false;
  RepeatingAlarmTimerTester f(&did_run, base::TimeDelta());
  f.Start();

  base::RunLoop().Run();

  EXPECT_TRUE(did_run);
}

TEST(AlarmTimerTest, RepeatingAlarmTimerZeroDelay_Cancel) {
  base::MessageLoopForIO loop;
  base::FileDescriptorWatcher file_descriptor_watcher(&loop);

  bool did_run_a = false;
  RepeatingAlarmTimerTester* a =
      new RepeatingAlarmTimerTester(&did_run_a, base::TimeDelta());

  // This should run before the timer expires.
  base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE, a);

  // Now start the timer.
  a->Start();

  bool did_run_b = false;
  RepeatingAlarmTimerTester b(&did_run_b, base::TimeDelta());
  b.Start();

  base::RunLoop().Run();

  EXPECT_FALSE(did_run_a);
  EXPECT_TRUE(did_run_b);
}

TEST(AlarmTimerTest, MessageLoopShutdown) {
  // This test is designed to verify that shutdown of the
  // message loop does not cause crashes if there were pending
  // timers not yet fired.  It may only trigger exceptions
  // if debug heap checking is enabled.
  bool did_run = false;
  {
    auto loop = std::make_unique<base::MessageLoopForIO>();
    auto file_descriptor_watcher =
        std::make_unique<base::FileDescriptorWatcher>(loop.get());
    OneShotAlarmTimerTester a(&did_run, kTenMilliseconds);
    OneShotAlarmTimerTester b(&did_run, kTenMilliseconds);
    OneShotAlarmTimerTester c(&did_run, kTenMilliseconds);
    OneShotAlarmTimerTester d(&did_run, kTenMilliseconds);

    a.Start();
    b.Start();

    // Allow FileDescriptorWatcher to start watching the timers. Without this,
    // tasks posted by FileDescriptorWatcher::WatchReadable() are leaked.
    base::RunLoop().RunUntilIdle();

    // MessageLoop and FileDescriptorWatcher destruct.
    file_descriptor_watcher.reset();
    loop.reset();
  }  // OneShotTimers destruct. SHOULD NOT CRASH, of course.

  EXPECT_FALSE(did_run);
}

TEST(AlarmTimerTest, NonRepeatIsRunning) {
  {
    base::MessageLoopForIO loop;
    base::FileDescriptorWatcher file_descriptor_watcher(&loop);
    timers::OneShotAlarmTimer timer;
    EXPECT_FALSE(timer.IsRunning());
    timer.Start(FROM_HERE, base::TimeDelta::FromDays(1), base::DoNothing());

    // Allow FileDescriptorWatcher to start watching the timer. Without this, a
    // task posted by FileDescriptorWatcher::WatchReadable() is leaked.
    base::RunLoop().RunUntilIdle();

    EXPECT_TRUE(timer.IsRunning());
    timer.Stop();
    EXPECT_FALSE(timer.IsRunning());
    EXPECT_TRUE(timer.user_task().is_null());
  }

  {
    base::MessageLoopForIO loop;
    base::FileDescriptorWatcher file_descriptor_watcher(&loop);
    timers::SimpleAlarmTimer timer;
    EXPECT_FALSE(timer.IsRunning());
    timer.Start(FROM_HERE, base::TimeDelta::FromDays(1), base::DoNothing());

    // Allow FileDescriptorWatcher to start watching the timer. Without this, a
    // task posted by FileDescriptorWatcher::WatchReadable() is leaked.
    base::RunLoop().RunUntilIdle();

    EXPECT_TRUE(timer.IsRunning());
    timer.Stop();
    EXPECT_FALSE(timer.IsRunning());
    ASSERT_FALSE(timer.user_task().is_null());
    timer.Reset();
    base::RunLoop().RunUntilIdle();
    EXPECT_TRUE(timer.IsRunning());
  }
}

TEST(AlarmTimerTest, RetainRepeatIsRunning) {
  base::MessageLoopForIO loop;
  base::FileDescriptorWatcher file_descriptor_watcher(&loop);
  timers::RepeatingAlarmTimer timer;
  EXPECT_FALSE(timer.IsRunning());
  timer.Start(FROM_HERE, base::TimeDelta::FromDays(1), base::DoNothing());

  // Allow FileDescriptorWatcher to start watching the timer. Without this, a
  // task posted by FileDescriptorWatcher::WatchReadable() is leaked.
  base::RunLoop().RunUntilIdle();

  EXPECT_TRUE(timer.IsRunning());
  timer.Reset();
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(timer.IsRunning());
  timer.Stop();
  EXPECT_FALSE(timer.IsRunning());
  timer.Reset();
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(timer.IsRunning());
}

TEST(AlarmTimerTest, RetainNonRepeatIsRunning) {
  base::MessageLoopForIO loop;
  base::FileDescriptorWatcher file_descriptor_watcher(&loop);
  timers::SimpleAlarmTimer timer;
  EXPECT_FALSE(timer.IsRunning());
  timer.Start(FROM_HERE, base::TimeDelta::FromDays(1), base::DoNothing());

  // Allow FileDescriptorWatcher to start watching the timer. Without this, a
  // task posted by FileDescriptorWatcher::WatchReadable() is leaked.
  base::RunLoop().RunUntilIdle();

  EXPECT_TRUE(timer.IsRunning());
  timer.Reset();
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(timer.IsRunning());
  timer.Stop();
  EXPECT_FALSE(timer.IsRunning());
  timer.Reset();
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(timer.IsRunning());
}

namespace {

bool g_callback_happened1 = false;
bool g_callback_happened2 = false;

void ClearAllCallbackHappened() {
  g_callback_happened1 = false;
  g_callback_happened2 = false;
}

void SetCallbackHappened1() {
  g_callback_happened1 = true;
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::RunLoop::QuitCurrentWhenIdleClosureDeprecated());
}

void SetCallbackHappened2() {
  g_callback_happened2 = true;
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::RunLoop::QuitCurrentWhenIdleClosureDeprecated());
}

TEST(AlarmTimerTest, ContinuationStopStart) {
  ClearAllCallbackHappened();
  base::MessageLoopForIO loop;
  base::FileDescriptorWatcher file_descriptor_watcher(&loop);
  timers::OneShotAlarmTimer timer;
  timer.Start(FROM_HERE, base::TimeDelta::FromMilliseconds(10),
              base::Bind(&SetCallbackHappened1));
  timer.Stop();
  timer.Start(FROM_HERE, base::TimeDelta::FromMilliseconds(40),
              base::Bind(&SetCallbackHappened2));
  base::RunLoop().Run();
  EXPECT_FALSE(g_callback_happened1);
  EXPECT_TRUE(g_callback_happened2);
}

TEST(AlarmTimerTest, ContinuationReset) {
  ClearAllCallbackHappened();
  base::MessageLoopForIO loop;
  base::FileDescriptorWatcher file_descriptor_watcher(&loop);
  timers::OneShotAlarmTimer timer;
  timer.Start(FROM_HERE, base::TimeDelta::FromMilliseconds(10),
              base::Bind(&SetCallbackHappened1));
  timer.Reset();
  // Since Reset happened before task ran, the user_task must not be cleared:
  ASSERT_FALSE(timer.user_task().is_null());
  base::RunLoop().Run();
  EXPECT_TRUE(g_callback_happened1);
}

// Verify that no crash occurs if a timer is deleted while its callback is
// running.
TEST(AlarmTimerTest, DeleteTimerWhileCallbackIsRunning) {
  base::MessageLoopForIO loop;
  base::FileDescriptorWatcher file_descriptor_watcher(&loop);
  base::RunLoop run_loop;

  // Will be deleted by the callback.
  timers::OneShotAlarmTimer* timer = new timers::OneShotAlarmTimer;

  timer->Start(
      FROM_HERE, base::TimeDelta::FromMilliseconds(10),
      base::Bind(
          [](timers::OneShotAlarmTimer* timer, base::RunLoop* run_loop) {
            delete timer;
            run_loop->Quit();
          },
          timer, &run_loop));
  run_loop.Run();
}

// Verify that no crash occurs if a zero-delay timer is deleted while its
// callback is running.
TEST(AlarmTimerTest, DeleteTimerWhileCallbackIsRunningZeroDelay) {
  base::MessageLoopForIO loop;
  base::FileDescriptorWatcher file_descriptor_watcher(&loop);
  base::RunLoop run_loop;

  // Will be deleted by the callback.
  timers::OneShotAlarmTimer* timer = new timers::OneShotAlarmTimer;

  timer->Start(
      FROM_HERE, base::TimeDelta(),
      base::Bind(
          [](timers::OneShotAlarmTimer* timer, base::RunLoop* run_loop) {
            delete timer;
            run_loop->Quit();
          },
          timer, &run_loop));
  run_loop.Run();
}

}  // namespace
}  // namespace timers
