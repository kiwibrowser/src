// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/metrics/thread_watcher.h"

#include <math.h>
#include <stdint.h>

#include <memory>

#include "base/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_tokenizer.h"
#include "base/synchronization/condition_variable.h"
#include "base/synchronization/lock.h"
#include "base/synchronization/spin_wait.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/platform_thread.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "chrome/common/chrome_switches.h"
#include "content/public/test/test_browser_thread.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

using base::TimeDelta;
using base::TimeTicks;
using content::BrowserThread;

enum State {
  INITIALIZED,        // Created ThreadWatch object.
  ACTIVATED,          // Thread watching activated.
  SENT_PING,          // Sent ping message to watched thread.
  RECEIVED_PONG,      // Received Pong message.
  DEACTIVATED,        // Thread watching de-activated.
};

enum WaitState {
  UNINITIALIZED,
  STARTED_WAITING,    // Start waiting for state_ to change to expected_state.
  STOPPED_WAITING,    // Done with the waiting.
  ALL_DONE,           // Done with waiting for STOPPED_WAITING.
};

enum CheckResponseState {
  UNKNOWN,
  SUCCESSFUL,         // CheckResponse was successful.
  FAILED,             // CheckResponse has failed.
};

// This class helps to track and manipulate thread state during tests. This
// class also has utility method to simulate hanging of watched thread by making
// the watched thread wait for a very long time by posting a task on watched
// thread that keeps it busy. It also has an utility method to block running of
// tests until ThreadWatcher object's post-condition state changes to an
// expected state.
class CustomThreadWatcher : public ThreadWatcher {
 public:
  base::Lock custom_lock_;
  base::ConditionVariable state_changed_;
  State thread_watcher_state_;
  WaitState wait_state_;
  CheckResponseState check_response_state_;
  uint64_t ping_sent_;
  uint64_t pong_received_;
  base::subtle::Atomic32 success_response_;
  base::subtle::Atomic32 failed_response_;
  base::TimeTicks saved_ping_time_;
  uint64_t saved_ping_sequence_number_;

  CustomThreadWatcher(const BrowserThread::ID thread_id,
                      const std::string thread_name,
                      const TimeDelta& sleep_time,
                      const TimeDelta& unresponsive_time)
      : ThreadWatcher(WatchingParams(thread_id,
                                     thread_name,
                                     sleep_time,
                                     unresponsive_time,
                                     ThreadWatcherList::kUnresponsiveCount,
                                     true)),
        state_changed_(&custom_lock_),
        thread_watcher_state_(INITIALIZED),
        wait_state_(UNINITIALIZED),
        check_response_state_(UNKNOWN),
        ping_sent_(0),
        pong_received_(0),
        success_response_(0),
        failed_response_(0),
        saved_ping_time_(base::TimeTicks::Now()),
        saved_ping_sequence_number_(0) {}

  State UpdateState(State new_state) {
    State old_state;
    {
      base::AutoLock auto_lock(custom_lock_);
      old_state = thread_watcher_state_;
      if (old_state != DEACTIVATED)
        thread_watcher_state_ = new_state;
      if (new_state == SENT_PING)
        ++ping_sent_;
      if (new_state == RECEIVED_PONG)
        ++pong_received_;
      saved_ping_time_ = ping_time();
      saved_ping_sequence_number_ = ping_sequence_number();
    }
    state_changed_.Broadcast();
    return old_state;
  }

  WaitState UpdateWaitState(WaitState new_state) {
    WaitState old_state;
    {
      base::AutoLock auto_lock(custom_lock_);
      old_state = wait_state_;
      wait_state_ = new_state;
    }
    state_changed_.Broadcast();
    return old_state;
  }

  void ActivateThreadWatching() override {
    State old_state = UpdateState(ACTIVATED);
    EXPECT_EQ(old_state, INITIALIZED);
    ThreadWatcher::ActivateThreadWatching();
  }

  void DeActivateThreadWatching() override {
    State old_state = UpdateState(DEACTIVATED);
    EXPECT_TRUE(old_state == ACTIVATED || old_state == SENT_PING ||
                old_state == RECEIVED_PONG);
    ThreadWatcher::DeActivateThreadWatching();
  }

  void PostPingMessage() override {
    State old_state = UpdateState(SENT_PING);
    EXPECT_TRUE(old_state == ACTIVATED || old_state == RECEIVED_PONG);
    ThreadWatcher::PostPingMessage();
  }

  void OnPongMessage(uint64_t ping_sequence_number) override {
    State old_state = UpdateState(RECEIVED_PONG);
    EXPECT_TRUE(old_state == SENT_PING || old_state == DEACTIVATED);
    ThreadWatcher::OnPongMessage(ping_sequence_number);
  }

  void OnCheckResponsiveness(uint64_t ping_sequence_number) override {
    ThreadWatcher::OnCheckResponsiveness(ping_sequence_number);
    {
      base::AutoLock auto_lock(custom_lock_);
      if (responsive_) {
        base::subtle::Release_Store(&success_response_,
            base::subtle::Acquire_Load(&success_response_) + 1);
        check_response_state_ = SUCCESSFUL;
      } else {
        base::subtle::Release_Store(&failed_response_,
            base::subtle::Acquire_Load(&failed_response_) + 1);
        check_response_state_ = FAILED;
      }
    }
    // Broadcast to indicate we have checked responsiveness of the thread that
    // is watched.
    state_changed_.Broadcast();
  }

  void WaitForWaitStateChange(TimeDelta wait_time, WaitState expected_state) {
    DCHECK(!WatchDogThread::CurrentlyOnWatchDogThread());
    TimeTicks end_time = TimeTicks::Now() + wait_time;
    {
      base::AutoLock auto_lock(custom_lock_);
      while (wait_state_ != expected_state && TimeTicks::Now() < end_time)
        state_changed_.TimedWait(end_time - TimeTicks::Now());
    }
  }

  void VeryLongMethod(TimeDelta wait_time) {
    DCHECK(!WatchDogThread::CurrentlyOnWatchDogThread());
    WaitForWaitStateChange(wait_time, STOPPED_WAITING);
    UpdateWaitState(ALL_DONE);
  }

  State WaitForStateChange(const TimeDelta& wait_time, State expected_state) {
    DCHECK(!WatchDogThread::CurrentlyOnWatchDogThread());
    UpdateWaitState(STARTED_WAITING);

    State exit_state = INITIALIZED;
    // Keep the thread that is running the tests waiting until ThreadWatcher
    // object's state changes to the expected_state or until wait_time elapses.
    for (uint32_t i = 0; i < unresponsive_threshold_; ++i) {
        TimeTicks end_time = TimeTicks::Now() + wait_time;
        {
          base::AutoLock auto_lock(custom_lock_);
          while (thread_watcher_state_ != expected_state &&
                 TimeTicks::Now() < end_time) {
            TimeDelta state_change_wait_time = end_time - TimeTicks::Now();
            state_changed_.TimedWait(state_change_wait_time);
          }
          // Capture the thread_watcher_state_ before it changes and return it
          // to the caller.
          exit_state = thread_watcher_state_;
          if (exit_state == expected_state)
            break;
        }
    }
    UpdateWaitState(STOPPED_WAITING);
    return exit_state;
  }

  CheckResponseState WaitForCheckResponse(const TimeDelta& wait_time,
                                          CheckResponseState expected_state) {
    DCHECK(!WatchDogThread::CurrentlyOnWatchDogThread());
    UpdateWaitState(STARTED_WAITING);

    CheckResponseState exit_state = UNKNOWN;
    // Keep the thread that is running the tests waiting until ThreadWatcher
    // object's check_response_state_ changes to the expected_state or until
    // wait_time elapses.
    for (uint32_t i = 0; i < unresponsive_threshold_; ++i) {
        TimeTicks end_time = TimeTicks::Now() + wait_time;
        {
          base::AutoLock auto_lock(custom_lock_);
          while (check_response_state_ != expected_state &&
                 TimeTicks::Now() < end_time) {
            TimeDelta state_change_wait_time = end_time - TimeTicks::Now();
            state_changed_.TimedWait(state_change_wait_time);
          }
          // Capture the check_response_state_ before it changes and return it
          // to the caller.
          exit_state = check_response_state_;
          if (exit_state == expected_state)
            break;
        }
    }
    UpdateWaitState(STOPPED_WAITING);
    return exit_state;
  }
};

class ThreadWatcherTest : public ::testing::Test {
 public:
  static constexpr TimeDelta kSleepTime = TimeDelta::FromMilliseconds(50);
  static constexpr TimeDelta kUnresponsiveTime =
      TimeDelta::FromMilliseconds(500);
  static constexpr char kIOThreadName[] = "IO";
  static constexpr char kUIThreadName[] = "UI";
  static constexpr char kCrashOnHangThreadNames[] = "UI,IO";
  static constexpr char kCrashOnHangThreadData[] = "UI:12,IO:12";

  CustomThreadWatcher* io_watcher_;
  CustomThreadWatcher* ui_watcher_;
  ThreadWatcherList* thread_watcher_list_;

  ThreadWatcherTest()
      : setup_complete_(&lock_),
        initialized_(false) {
    ui_thread_.reset(new content::TestBrowserThread(BrowserThread::UI));
    io_thread_.reset(new content::TestBrowserThread(BrowserThread::IO));
    watchdog_thread_.reset(new WatchDogThread());
    ui_thread_->StartAndWaitForTesting();
    io_thread_->StartAndWaitForTesting();
    watchdog_thread_->StartAndWaitForTesting();

    WatchDogThread::PostTask(
        FROM_HERE,
        base::Bind(&ThreadWatcherTest::SetUpObjects, base::Unretained(this)));

    WaitForSetUp(TimeDelta::FromMinutes(1));
  }

  void SetUpObjects() {
    DCHECK(WatchDogThread::CurrentlyOnWatchDogThread());

    // Setup the registry for thread watchers.
    thread_watcher_list_ = new ThreadWatcherList();

    // Create thread watcher object for the IO thread.
    std::unique_ptr<CustomThreadWatcher> io_watcher(
        new CustomThreadWatcher(BrowserThread::IO, kIOThreadName,
                                kSleepTime, kUnresponsiveTime));
    io_watcher_ = io_watcher.get();
    ThreadWatcher* registered_io_watcher =
        ThreadWatcherList::Register(std::move(io_watcher));
    EXPECT_EQ(io_watcher_, registered_io_watcher);
    EXPECT_EQ(io_watcher_, thread_watcher_list_->Find(BrowserThread::IO));

    // Create thread watcher object for the UI thread.
    std::unique_ptr<CustomThreadWatcher> ui_watcher(new CustomThreadWatcher(
        BrowserThread::UI, kUIThreadName, kSleepTime, kUnresponsiveTime));
    ui_watcher_ = ui_watcher.get();
    ThreadWatcher* registered_ui_watcher =
        ThreadWatcherList::Register(std::move(ui_watcher));
    EXPECT_EQ(ui_watcher_, registered_ui_watcher);
    EXPECT_EQ(ui_watcher_, thread_watcher_list_->Find(BrowserThread::UI));

    {
      base::AutoLock lock(lock_);
      initialized_ = true;
    }
    setup_complete_.Signal();
  }

  void WaitForSetUp(TimeDelta wait_time) {
    DCHECK(!WatchDogThread::CurrentlyOnWatchDogThread());
    TimeTicks end_time = TimeTicks::Now() + wait_time;
    {
      base::AutoLock auto_lock(lock_);
      while (!initialized_ && TimeTicks::Now() < end_time)
        setup_complete_.TimedWait(end_time - TimeTicks::Now());
    }
  }

  ~ThreadWatcherTest() override {
    ThreadWatcherList::DeleteAll();
    io_watcher_ = nullptr;
    ui_watcher_ = nullptr;
    io_thread_.reset();
    ui_thread_.reset();
    watchdog_thread_.reset();
    thread_watcher_list_ = nullptr;
  }

 private:
  base::MessageLoop message_loop_;
  base::Lock lock_;
  base::ConditionVariable setup_complete_;
  bool initialized_;
  std::unique_ptr<content::TestBrowserThread> ui_thread_;
  std::unique_ptr<content::TestBrowserThread> io_thread_;
  std::unique_ptr<WatchDogThread> watchdog_thread_;

  DISALLOW_COPY_AND_ASSIGN(ThreadWatcherTest);
};

// Declare storage for ThreadWatcherTest's static constants.
constexpr TimeDelta ThreadWatcherTest::kSleepTime;
constexpr TimeDelta ThreadWatcherTest::kUnresponsiveTime;
constexpr char ThreadWatcherTest::kIOThreadName[];
constexpr char ThreadWatcherTest::kUIThreadName[];
constexpr char ThreadWatcherTest::kCrashOnHangThreadNames[];
constexpr char ThreadWatcherTest::kCrashOnHangThreadData[];

TEST_F(ThreadWatcherTest, ThreadNamesOnlyArgs) {
  // Setup command_line arguments.
  base::CommandLine command_line(base::CommandLine::NO_PROGRAM);
  command_line.AppendSwitchASCII(switches::kCrashOnHangThreads,
                                 kCrashOnHangThreadNames);

  // Parse command_line arguments.
  ThreadWatcherList::CrashOnHangThreadMap crash_on_hang_threads;
  uint32_t unresponsive_threshold;
  ThreadWatcherList::ParseCommandLine(command_line,
                                      &unresponsive_threshold,
                                      &crash_on_hang_threads);

  // Verify the data.
  base::CStringTokenizer tokens(
      kCrashOnHangThreadNames,
      kCrashOnHangThreadNames + (arraysize(kCrashOnHangThreadNames) - 1), ",");
  while (tokens.GetNext()) {
    std::vector<base::StringPiece> values = base::SplitStringPiece(
        tokens.token_piece(), ":", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
    std::string thread_name = values[0].as_string();

    ThreadWatcherList::CrashOnHangThreadMap::iterator it =
        crash_on_hang_threads.find(thread_name);
    bool crash_on_hang = (it != crash_on_hang_threads.end());
    EXPECT_TRUE(crash_on_hang);
    EXPECT_LT(0u, it->second);
  }
}

TEST_F(ThreadWatcherTest, CrashOnHangThreadsAllArgs) {
  // Setup command_line arguments.
  base::CommandLine command_line(base::CommandLine::NO_PROGRAM);
  command_line.AppendSwitchASCII(switches::kCrashOnHangThreads,
                                 kCrashOnHangThreadData);

  // Parse command_line arguments.
  ThreadWatcherList::CrashOnHangThreadMap crash_on_hang_threads;
  uint32_t unresponsive_threshold;
  ThreadWatcherList::ParseCommandLine(command_line,
                                      &unresponsive_threshold,
                                      &crash_on_hang_threads);

  // Verify the data.
  base::CStringTokenizer tokens(
      kCrashOnHangThreadData,
      kCrashOnHangThreadData + (arraysize(kCrashOnHangThreadData) - 1), ",");
  while (tokens.GetNext()) {
    std::vector<base::StringPiece> values = base::SplitStringPiece(
        tokens.token_piece(), ":", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
    std::string thread_name = values[0].as_string();

    ThreadWatcherList::CrashOnHangThreadMap::iterator it =
        crash_on_hang_threads.find(thread_name);

    bool crash_on_hang = (it != crash_on_hang_threads.end());
    EXPECT_TRUE(crash_on_hang);

    uint32_t crash_unresponsive_threshold = it->second;
    uint32_t crash_on_unresponsive_seconds =
        ThreadWatcherList::kUnresponsiveSeconds * crash_unresponsive_threshold;
    EXPECT_EQ(12u, crash_on_unresponsive_seconds);
  }
}

// Test registration. When thread_watcher_list_ goes out of scope after
// TearDown, all thread watcher objects will be deleted.
TEST_F(ThreadWatcherTest, Registration) {
  // Check ThreadWatcher object has all correct parameters.
  EXPECT_EQ(BrowserThread::IO, io_watcher_->thread_id());
  EXPECT_EQ(kIOThreadName, io_watcher_->thread_name());
  EXPECT_EQ(kSleepTime, io_watcher_->sleep_time());
  EXPECT_EQ(kUnresponsiveTime, io_watcher_->unresponsive_time());
  EXPECT_FALSE(io_watcher_->active());

  // Check ThreadWatcher object of watched UI thread has correct data.
  EXPECT_EQ(BrowserThread::UI, ui_watcher_->thread_id());
  EXPECT_EQ(kUIThreadName, ui_watcher_->thread_name());
  EXPECT_EQ(kSleepTime, ui_watcher_->sleep_time());
  EXPECT_EQ(kUnresponsiveTime, ui_watcher_->unresponsive_time());
  EXPECT_FALSE(ui_watcher_->active());
}

// Test ActivateThreadWatching and DeActivateThreadWatching of IO thread. This
// method also checks that pong message was sent by the watched thread and pong
// message was received by the WatchDogThread. It also checks that
// OnCheckResponsiveness has verified the ping-pong mechanism and the watched
// thread is not hung.
TEST_F(ThreadWatcherTest, ThreadResponding) {
  TimeTicks time_before_ping = TimeTicks::Now();
  // Activate watching IO thread.
  WatchDogThread::PostTask(
      FROM_HERE,
      base::Bind(&ThreadWatcher::ActivateThreadWatching,
                 base::Unretained(io_watcher_)));

  // Activate would have started ping/pong messaging. Expect atleast one
  // ping/pong messaging sequence to happen.
  io_watcher_->WaitForStateChange(kSleepTime + TimeDelta::FromMinutes(1),
                                  RECEIVED_PONG);
  EXPECT_GT(io_watcher_->ping_sent_, static_cast<uint64_t>(0));
  EXPECT_GT(io_watcher_->pong_received_, static_cast<uint64_t>(0));
  EXPECT_TRUE(io_watcher_->active());
  EXPECT_GE(io_watcher_->saved_ping_time_, time_before_ping);
  EXPECT_GE(io_watcher_->saved_ping_sequence_number_, static_cast<uint64_t>(0));

  // Verify watched thread is responding with ping/pong messaging.
  io_watcher_->WaitForCheckResponse(
      kUnresponsiveTime + TimeDelta::FromMinutes(1), SUCCESSFUL);
  EXPECT_GT(base::subtle::NoBarrier_Load(&(io_watcher_->success_response_)),
      static_cast<base::subtle::Atomic32>(0));
  EXPECT_EQ(base::subtle::NoBarrier_Load(&(io_watcher_->failed_response_)),
      static_cast<base::subtle::Atomic32>(0));

  // DeActivate thread watching for shutdown.
  WatchDogThread::PostTask(
      FROM_HERE,
      base::Bind(&ThreadWatcher::DeActivateThreadWatching,
      base::Unretained(io_watcher_)));
}

// This test posts a task on watched thread that takes very long time (this is
// to simulate hanging of watched thread). It then checks for
// OnCheckResponsiveness raising an alert (OnCheckResponsiveness returns false
// if the watched thread is not responding).
TEST_F(ThreadWatcherTest, ThreadNotResponding) {
  // Simulate hanging of watched thread by making the watched thread wait for a
  // very long time by posting a task on watched thread that keeps it busy.
  // It is safe to use base::Unretained because test is waiting for the method
  // to finish.
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&CustomThreadWatcher::VeryLongMethod,
                     base::Unretained(io_watcher_), kUnresponsiveTime * 10));

  // Activate thread watching.
  WatchDogThread::PostTask(
      FROM_HERE,
      base::Bind(&ThreadWatcher::ActivateThreadWatching,
                 base::Unretained(io_watcher_)));

  // Verify watched thread is not responding for ping messages.
  io_watcher_->WaitForCheckResponse(
      kUnresponsiveTime + TimeDelta::FromMinutes(1), FAILED);
  EXPECT_EQ(base::subtle::NoBarrier_Load(&(io_watcher_->success_response_)),
      static_cast<base::subtle::Atomic32>(0));
  EXPECT_GT(base::subtle::NoBarrier_Load(&(io_watcher_->failed_response_)),
      static_cast<base::subtle::Atomic32>(0));

  // DeActivate thread watching for shutdown.
  WatchDogThread::PostTask(
      FROM_HERE,
      base::Bind(&ThreadWatcher::DeActivateThreadWatching,
                 base::Unretained(io_watcher_)));

  // Wait for the io_watcher_'s VeryLongMethod to finish.
  io_watcher_->WaitForWaitStateChange(kUnresponsiveTime * 10, ALL_DONE);
}

// Test watching of multiple threads with all threads not responding.
TEST_F(ThreadWatcherTest, MultipleThreadsResponding) {
  // Check for UI thread to perform ping/pong messaging.
  WatchDogThread::PostTask(FROM_HERE,
                           base::Bind(&ThreadWatcher::ActivateThreadWatching,
                                      base::Unretained(ui_watcher_)));

  // Check for IO thread to perform ping/pong messaging.
  WatchDogThread::PostTask(
      FROM_HERE,
      base::Bind(&ThreadWatcher::ActivateThreadWatching,
                 base::Unretained(io_watcher_)));

  // Verify UI thread is responding with ping/pong messaging.
  ui_watcher_->WaitForCheckResponse(
      kUnresponsiveTime + TimeDelta::FromMinutes(1), SUCCESSFUL);
  EXPECT_GT(ui_watcher_->ping_sent_, static_cast<uint64_t>(0));
  EXPECT_GT(ui_watcher_->pong_received_, static_cast<uint64_t>(0));
  EXPECT_GE(ui_watcher_->ping_sequence_number_, static_cast<uint64_t>(0));
  EXPECT_GT(base::subtle::NoBarrier_Load(&(ui_watcher_->success_response_)),
            static_cast<base::subtle::Atomic32>(0));
  EXPECT_EQ(base::subtle::NoBarrier_Load(&(ui_watcher_->failed_response_)),
            static_cast<base::subtle::Atomic32>(0));

  // Verify IO thread is responding with ping/pong messaging.
  io_watcher_->WaitForCheckResponse(
      kUnresponsiveTime + TimeDelta::FromMinutes(1), SUCCESSFUL);
  EXPECT_GT(io_watcher_->ping_sent_, static_cast<uint64_t>(0));
  EXPECT_GT(io_watcher_->pong_received_, static_cast<uint64_t>(0));
  EXPECT_GE(io_watcher_->ping_sequence_number_, static_cast<uint64_t>(0));
  EXPECT_GT(base::subtle::NoBarrier_Load(&(io_watcher_->success_response_)),
      static_cast<base::subtle::Atomic32>(0));
  EXPECT_EQ(base::subtle::NoBarrier_Load(&(io_watcher_->failed_response_)),
      static_cast<base::subtle::Atomic32>(0));

  // DeActivate thread watching for shutdown.
  WatchDogThread::PostTask(
      FROM_HERE,
      base::Bind(&ThreadWatcher::DeActivateThreadWatching,
                 base::Unretained(io_watcher_)));

  WatchDogThread::PostTask(FROM_HERE,
                           base::Bind(&ThreadWatcher::DeActivateThreadWatching,
                                      base::Unretained(ui_watcher_)));
}

// Test watching of multiple threads with one of the threads not responding.
TEST_F(ThreadWatcherTest, MultipleThreadsNotResponding) {
  // Simulate hanging of watched thread by making the watched thread wait for a
  // very long time by posting a task on watched thread that keeps it busy.
  // It is safe ot use base::Unretained because test is waiting for the method
  // to finish.
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&CustomThreadWatcher::VeryLongMethod,
                     base::Unretained(io_watcher_), kUnresponsiveTime * 10));

  // Activate watching of UI thread.
  WatchDogThread::PostTask(FROM_HERE,
                           base::Bind(&ThreadWatcher::ActivateThreadWatching,
                                      base::Unretained(ui_watcher_)));

  // Activate watching of IO thread.
  WatchDogThread::PostTask(
      FROM_HERE,
      base::Bind(&ThreadWatcher::ActivateThreadWatching,
                 base::Unretained(io_watcher_)));

  // Verify UI thread is responding with ping/pong messaging.
  ui_watcher_->WaitForCheckResponse(
      kUnresponsiveTime + TimeDelta::FromMinutes(1), SUCCESSFUL);
  EXPECT_GT(base::subtle::NoBarrier_Load(&(ui_watcher_->success_response_)),
            static_cast<base::subtle::Atomic32>(0));
  EXPECT_EQ(base::subtle::NoBarrier_Load(&(ui_watcher_->failed_response_)),
            static_cast<base::subtle::Atomic32>(0));

  // Verify IO thread is not responding for ping messages.
  io_watcher_->WaitForCheckResponse(
      kUnresponsiveTime + TimeDelta::FromMinutes(1), FAILED);
  EXPECT_EQ(base::subtle::NoBarrier_Load(&(io_watcher_->success_response_)),
      static_cast<base::subtle::Atomic32>(0));
  EXPECT_GT(base::subtle::NoBarrier_Load(&(io_watcher_->failed_response_)),
      static_cast<base::subtle::Atomic32>(0));

  // DeActivate thread watching for shutdown.
  WatchDogThread::PostTask(
      FROM_HERE,
      base::Bind(&ThreadWatcher::DeActivateThreadWatching,
                 base::Unretained(io_watcher_)));
  WatchDogThread::PostTask(FROM_HERE,
                           base::Bind(&ThreadWatcher::DeActivateThreadWatching,
                                      base::Unretained(ui_watcher_)));

  // Wait for the io_watcher_'s VeryLongMethod to finish.
  io_watcher_->WaitForWaitStateChange(kUnresponsiveTime * 10, ALL_DONE);
}

class ThreadWatcherListTest : public ::testing::Test {
 protected:
  ThreadWatcherListTest()
      : done_(&lock_),
        state_available_(false),
        has_thread_watcher_list_(false),
        stopped_(false) {
  }

  void ReadStateOnWatchDogThread() {
    CHECK(WatchDogThread::CurrentlyOnWatchDogThread());
    {
      base::AutoLock auto_lock(lock_);
      has_thread_watcher_list_ =
          ThreadWatcherList::g_thread_watcher_list_ != nullptr;
      stopped_ = ThreadWatcherList::g_stopped_;
      state_available_ = true;
    }
    done_.Signal();
  }

  void CheckState(bool has_thread_watcher_list,
                  bool stopped,
                  const char* const msg) {
    CHECK(!WatchDogThread::CurrentlyOnWatchDogThread());
    {
      base::AutoLock auto_lock(lock_);
      state_available_ = false;
    }

    WatchDogThread::PostTask(
        FROM_HERE,
        base::Bind(&ThreadWatcherListTest::ReadStateOnWatchDogThread,
                   base::Unretained(this)));
    {
      base::AutoLock auto_lock(lock_);
      while (!state_available_)
        done_.Wait();

      EXPECT_EQ(has_thread_watcher_list, has_thread_watcher_list_) << msg;
      EXPECT_EQ(stopped, stopped_) << msg;
    }
  }

  base::Lock lock_;
  base::ConditionVariable done_;

  bool state_available_;
  bool has_thread_watcher_list_;
  bool stopped_;
};

TEST_F(ThreadWatcherListTest, Restart) {
  ThreadWatcherList::g_initialize_delay_seconds = 1;

  base::MessageLoopForUI message_loop_for_ui;
  content::TestBrowserThread ui_thread(BrowserThread::UI, &message_loop_for_ui);

  std::unique_ptr<WatchDogThread> watchdog_thread_(new WatchDogThread());
  watchdog_thread_->StartAndWaitForTesting();

  // See http://crbug.com/347887.
  // StartWatchingAll() will PostDelayedTask to create g_thread_watcher_list_,
  // whilst StopWatchingAll() will just PostTask to destroy it.
  // Ensure that when Stop is called, Start will NOT create
  // g_thread_watcher_list_ later on.
  ThreadWatcherList::StartWatchingAll(*base::CommandLine::ForCurrentProcess());
  ThreadWatcherList::StopWatchingAll();
  {
    base::RunLoop run_loop;
    message_loop_for_ui.task_runner()->PostDelayedTask(
        FROM_HERE, run_loop.QuitWhenIdleClosure(),
        base::TimeDelta::FromSeconds(
            ThreadWatcherList::g_initialize_delay_seconds));
    run_loop.Run();
  }

  CheckState(false /* has_thread_watcher_list */,
             true /* stopped */,
             "Start / Stopped");

  // Proceed with just |StartWatchingAll| and ensure it'll be started.
  ThreadWatcherList::StartWatchingAll(*base::CommandLine::ForCurrentProcess());
  {
    base::RunLoop run_loop;
    message_loop_for_ui.task_runner()->PostDelayedTask(
        FROM_HERE, run_loop.QuitWhenIdleClosure(),
        base::TimeDelta::FromSeconds(
            ThreadWatcherList::g_initialize_delay_seconds + 1));
    run_loop.Run();
  }

  CheckState(true /* has_thread_watcher_list */,
             false /* stopped */,
             "Started");

  // Finally, StopWatchingAll() must stop.
  ThreadWatcherList::StopWatchingAll();
  {
    base::RunLoop run_loop;
    message_loop_for_ui.task_runner()->PostDelayedTask(
        FROM_HERE, run_loop.QuitWhenIdleClosure(),
        base::TimeDelta::FromSeconds(
            ThreadWatcherList::g_initialize_delay_seconds));
    run_loop.Run();
  }

  CheckState(false /* has_thread_watcher_list */,
             true /* stopped */,
             "Stopped");
}
