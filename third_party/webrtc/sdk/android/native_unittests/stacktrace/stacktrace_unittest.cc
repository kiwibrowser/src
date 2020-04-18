/*
 *  Copyright 2019 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "sdk/android/native_api/stacktrace/stacktrace.h"
#include <dlfcn.h>
#include <vector>
#include "absl/memory/memory.h"
#include "rtc_base/critical_section.h"
#include "rtc_base/event.h"
#include "rtc_base/logging.h"
#include "rtc_base/platform_thread.h"
#include "rtc_base/strings/string_builder.h"
#include "test/gtest.h"

namespace webrtc {
namespace test {

// Returns the execution address relative to the .so base address. This matches
// the addresses we get from GetStacktrace().
uint32_t GetCurrentRelativeExecutionAddress() {
  void* pc = __builtin_return_address(0);
  Dl_info dl_info = {};
  const bool success = dladdr(pc, &dl_info);
  EXPECT_TRUE(success);
  return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pc) -
                               reinterpret_cast<uintptr_t>(dl_info.dli_fbase));
}

// Returns true if any of the stack trace element is inside the specified
// region.
bool StackTraceContainsRange(const std::vector<StackTraceElement>& stack_trace,
                             uintptr_t pc_low,
                             uintptr_t pc_high) {
  for (const StackTraceElement& stack_trace_element : stack_trace) {
    if (pc_low <= stack_trace_element.relative_address &&
        pc_high >= stack_trace_element.relative_address) {
      return true;
    }
  }
  return false;
}

class DeadlockInterface {
 public:
  virtual ~DeadlockInterface() {}

  // This function should deadlock until Release() is called.
  virtual void Deadlock() = 0;

  // This function should release the thread stuck in Deadlock().
  virtual void Release() = 0;
};

struct ThreadParams {
  volatile int tid;
  // Signaled when the deadlock region is entered.
  rtc::Event deadlock_start_event;
  DeadlockInterface* volatile deadlock_impl;
  // Defines an address range within the deadlock will occur.
  volatile uint32_t deadlock_region_start_address;
  volatile uint32_t deadlock_region_end_address;
  // Signaled when the deadlock is done.
  rtc::Event deadlock_done_event;
};

class RtcEventDeadlock : public DeadlockInterface {
 private:
  void Deadlock() override { event.Wait(rtc::Event::kForever); }
  void Release() override { event.Set(); }

  rtc::Event event;
};

class RtcCriticalSectionDeadlock : public DeadlockInterface {
 public:
  RtcCriticalSectionDeadlock()
      : critscope_(absl::make_unique<rtc::CritScope>(&crit_)) {}

 private:
  void Deadlock() override { rtc::CritScope lock(&crit_); }

  void Release() override { critscope_.reset(); }

  rtc::CriticalSection crit_;
  std::unique_ptr<rtc::CritScope> critscope_;
};

class SpinDeadlock : public DeadlockInterface {
 public:
  SpinDeadlock() : is_deadlocked_(true) {}

 private:
  void Deadlock() override {
    while (is_deadlocked_) {
    }
  }

  void Release() override { is_deadlocked_ = false; }

  std::atomic<bool> is_deadlocked_;
};

class SleepDeadlock : public DeadlockInterface {
 private:
  void Deadlock() override { sleep(1000000); }

  void Release() override {
    // The interrupt itself will break free from the sleep.
  }
};

// This is the function that is exectued by the thread that will deadlock and
// have its stacktrace captured.
void ThreadFunction(void* void_params) {
  ThreadParams* params = static_cast<ThreadParams*>(void_params);
  params->tid = gettid();

  params->deadlock_region_start_address = GetCurrentRelativeExecutionAddress();
  params->deadlock_start_event.Set();
  params->deadlock_impl->Deadlock();
  params->deadlock_region_end_address = GetCurrentRelativeExecutionAddress();

  params->deadlock_done_event.Set();
}

void TestStacktrace(std::unique_ptr<DeadlockInterface> deadlock_impl) {
  // Set params that will be sent to other thread.
  ThreadParams params;
  params.deadlock_impl = deadlock_impl.get();

  // Spawn thread.
  rtc::PlatformThread thread(&ThreadFunction, &params, "StacktraceTest");
  thread.Start();

  // Wait until the thread has entered the deadlock region.
  params.deadlock_start_event.Wait(rtc::Event::kForever);

  // Acquire the stack trace of the thread which should now be deadlocking.
  std::vector<StackTraceElement> stack_trace = GetStackTrace(params.tid);

  // Release the deadlock so that the thread can continue.
  deadlock_impl->Release();

  // Wait until the thread has left the deadlock.
  params.deadlock_done_event.Wait(rtc::Event::kForever);

  // Assert that the stack trace contains the deadlock region.
  EXPECT_TRUE(StackTraceContainsRange(stack_trace,
                                      params.deadlock_region_start_address,
                                      params.deadlock_region_end_address))
      << "Deadlock region: ["
      << rtc::ToHex(params.deadlock_region_start_address) << ", "
      << rtc::ToHex(params.deadlock_region_end_address)
      << "] not contained in: " << StackTraceToString(stack_trace);

  thread.Stop();
}

TEST(Stacktrace, TestSpinLock) {
  TestStacktrace(absl::make_unique<SpinDeadlock>());
}

TEST(Stacktrace, TestSleep) {
  TestStacktrace(absl::make_unique<SleepDeadlock>());
}

// Stack traces originating from kernel space does not include user space stack
// traces for ARM 32.
#ifdef WEBRTC_ARCH_ARM64
TEST(Stacktrace, TestRtcEvent) {
  TestStacktrace(absl::make_unique<RtcEventDeadlock>());
}

TEST(Stacktrace, TestRtcCriticalSection) {
  TestStacktrace(absl::make_unique<RtcCriticalSectionDeadlock>());
}
#endif

}  // namespace test
}  // namespace webrtc
