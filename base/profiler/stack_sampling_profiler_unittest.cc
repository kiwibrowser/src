// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>

#include <algorithm>
#include <cstdlib>
#include <memory>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/compiler_specific.h"
#include "base/files/file_util.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/native_library.h"
#include "base/path_service.h"
#include "base/profiler/native_stack_sampler.h"
#include "base/profiler/stack_sampling_profiler.h"
#include "base/run_loop.h"
#include "base/scoped_native_library.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/synchronization/lock.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/platform_thread.h"
#include "base/threading/simple_thread.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "testing/gtest/include/gtest/gtest.h"

#if defined(OS_WIN)
#include <intrin.h>
#include <malloc.h>
#include <windows.h>
#else
#include <alloca.h>
#endif

// STACK_SAMPLING_PROFILER_SUPPORTED is used to conditionally enable the tests
// below for supported platforms (currently Win x64 and Mac x64).
#if defined(_WIN64) || (defined(OS_MACOSX) && !defined(OS_IOS))
#define STACK_SAMPLING_PROFILER_SUPPORTED 1
#endif

#if defined(OS_WIN)
#pragma intrinsic(_ReturnAddress)
#endif

namespace base {

#if defined(STACK_SAMPLING_PROFILER_SUPPORTED)
#define PROFILER_TEST_F(TestClass, TestName) TEST_F(TestClass, TestName)
#else
#define PROFILER_TEST_F(TestClass, TestName) \
  TEST_F(TestClass, DISABLED_##TestName)
#endif

using SamplingParams = StackSamplingProfiler::SamplingParams;
using Frame = StackSamplingProfiler::Frame;
using Frames = std::vector<StackSamplingProfiler::Frame>;
using Module = StackSamplingProfiler::Module;
using Sample = StackSamplingProfiler::Sample;
using CallStackProfile = StackSamplingProfiler::CallStackProfile;
using CallStackProfiles = StackSamplingProfiler::CallStackProfiles;

namespace {

// Configuration for the frames that appear on the stack.
struct StackConfiguration {
  enum Config { NORMAL, WITH_ALLOCA, WITH_OTHER_LIBRARY };

  explicit StackConfiguration(Config config)
      : StackConfiguration(config, nullptr) {
    EXPECT_NE(config, WITH_OTHER_LIBRARY);
  }

  StackConfiguration(Config config, NativeLibrary library)
      : config(config), library(library) {
    EXPECT_TRUE(config != WITH_OTHER_LIBRARY || library);
  }

  Config config;

  // Only used if config == WITH_OTHER_LIBRARY.
  NativeLibrary library;
};

// Signature for a target function that is expected to appear in the stack. See
// SignalAndWaitUntilSignaled() below. The return value should be a program
// counter pointer near the end of the function.
using TargetFunction = const void*(*)(WaitableEvent*, WaitableEvent*,
                                      const StackConfiguration*);

// A thread to target for profiling, whose stack is guaranteed to contain
// SignalAndWaitUntilSignaled() when coordinated with the main thread.
class TargetThread : public PlatformThread::Delegate {
 public:
  explicit TargetThread(const StackConfiguration& stack_config);

  // PlatformThread::Delegate:
  void ThreadMain() override;

  // Waits for the thread to have started and be executing in
  // SignalAndWaitUntilSignaled().
  void WaitForThreadStart();

  // Allows the thread to return from SignalAndWaitUntilSignaled() and finish
  // execution.
  void SignalThreadToFinish();

  // This function is guaranteed to be executing between calls to
  // WaitForThreadStart() and SignalThreadToFinish() when invoked with
  // |thread_started_event_| and |finish_event_|. Returns a program counter
  // value near the end of the function. May be invoked with null WaitableEvents
  // to just return the program counter.
  //
  // This function is static so that we can get a straightforward address
  // for it in one of the tests below, rather than dealing with the complexity
  // of a member function pointer representation.
  static const void* SignalAndWaitUntilSignaled(
      WaitableEvent* thread_started_event,
      WaitableEvent* finish_event,
      const StackConfiguration* stack_config);

  // Calls into SignalAndWaitUntilSignaled() after allocating memory on the
  // stack with alloca.
  static const void* CallWithAlloca(WaitableEvent* thread_started_event,
                                    WaitableEvent* finish_event,
                                    const StackConfiguration* stack_config);

  // Calls into SignalAndWaitUntilSignaled() via a function in
  // base_profiler_test_support_library.
  static const void* CallThroughOtherLibrary(
      WaitableEvent* thread_started_event,
      WaitableEvent* finish_event,
      const StackConfiguration* stack_config);

  PlatformThreadId id() const { return id_; }

 private:
  struct TargetFunctionArgs {
    WaitableEvent* thread_started_event;
    WaitableEvent* finish_event;
    const StackConfiguration* stack_config;
  };

  // Callback function to be provided when calling through the other library.
  static void OtherLibraryCallback(void *arg);

  // Returns the current program counter, or a value very close to it.
  static const void* GetProgramCounter();

  WaitableEvent thread_started_event_;
  WaitableEvent finish_event_;
  PlatformThreadId id_;
  const StackConfiguration stack_config_;

  DISALLOW_COPY_AND_ASSIGN(TargetThread);
};

TargetThread::TargetThread(const StackConfiguration& stack_config)
    : thread_started_event_(WaitableEvent::ResetPolicy::AUTOMATIC,
                            WaitableEvent::InitialState::NOT_SIGNALED),
      finish_event_(WaitableEvent::ResetPolicy::AUTOMATIC,
                    WaitableEvent::InitialState::NOT_SIGNALED),
      id_(0),
      stack_config_(stack_config) {}

void TargetThread::ThreadMain() {
  id_ = PlatformThread::CurrentId();
  switch (stack_config_.config) {
    case StackConfiguration::NORMAL:
      SignalAndWaitUntilSignaled(&thread_started_event_, &finish_event_,
                                 &stack_config_);
      break;

    case StackConfiguration::WITH_ALLOCA:
      CallWithAlloca(&thread_started_event_, &finish_event_, &stack_config_);
      break;

    case StackConfiguration::WITH_OTHER_LIBRARY:
      CallThroughOtherLibrary(&thread_started_event_, &finish_event_,
                              &stack_config_);
      break;
  }
}

void TargetThread::WaitForThreadStart() {
  thread_started_event_.Wait();
}

void TargetThread::SignalThreadToFinish() {
  finish_event_.Signal();
}

// static
// Disable inlining for this function so that it gets its own stack frame.
NOINLINE const void* TargetThread::SignalAndWaitUntilSignaled(
    WaitableEvent* thread_started_event,
    WaitableEvent* finish_event,
    const StackConfiguration* stack_config) {
  if (thread_started_event && finish_event) {
    thread_started_event->Signal();
    finish_event->Wait();
  }

  // Volatile to prevent a tail call to GetProgramCounter().
  const void* volatile program_counter = GetProgramCounter();
  return program_counter;
}

// static
// Disable inlining for this function so that it gets its own stack frame.
NOINLINE const void* TargetThread::CallWithAlloca(
    WaitableEvent* thread_started_event,
    WaitableEvent* finish_event,
    const StackConfiguration* stack_config) {
  const size_t alloca_size = 100;
  // Memset to 0 to generate a clean failure.
  std::memset(alloca(alloca_size), 0, alloca_size);

  SignalAndWaitUntilSignaled(thread_started_event, finish_event, stack_config);

  // Volatile to prevent a tail call to GetProgramCounter().
  const void* volatile program_counter = GetProgramCounter();
  return program_counter;
}

// static
NOINLINE const void* TargetThread::CallThroughOtherLibrary(
    WaitableEvent* thread_started_event,
    WaitableEvent* finish_event,
    const StackConfiguration* stack_config) {
  if (stack_config) {
    // A function whose arguments are a function accepting void*, and a void*.
    using InvokeCallbackFunction = void(*)(void (*)(void*), void*);
    EXPECT_TRUE(stack_config->library);
    InvokeCallbackFunction function = reinterpret_cast<InvokeCallbackFunction>(
        GetFunctionPointerFromNativeLibrary(stack_config->library,
                                            "InvokeCallbackFunction"));
    EXPECT_TRUE(function);

    TargetFunctionArgs args = {
      thread_started_event,
      finish_event,
      stack_config
    };
    (*function)(&OtherLibraryCallback, &args);
  }

  // Volatile to prevent a tail call to GetProgramCounter().
  const void* volatile program_counter = GetProgramCounter();
  return program_counter;
}

// static
void TargetThread::OtherLibraryCallback(void *arg) {
  const TargetFunctionArgs* args = static_cast<TargetFunctionArgs*>(arg);
  SignalAndWaitUntilSignaled(args->thread_started_event, args->finish_event,
                             args->stack_config);
  // Prevent tail call.
  volatile int i = 0;
  ALLOW_UNUSED_LOCAL(i);
}

// static
// Disable inlining for this function so that it gets its own stack frame.
NOINLINE const void* TargetThread::GetProgramCounter() {
#if defined(OS_WIN)
  return _ReturnAddress();
#else
  return __builtin_return_address(0);
#endif
}

// Loads the other library, which defines a function to be called in the
// WITH_OTHER_LIBRARY configuration.
NativeLibrary LoadOtherLibrary() {
  // The lambda gymnastics works around the fact that we can't use ASSERT_*
  // macros in a function returning non-null.
  const auto load = [](NativeLibrary* library) {
    FilePath other_library_path;
    ASSERT_TRUE(PathService::Get(DIR_EXE, &other_library_path));
    other_library_path = other_library_path.AppendASCII(
        GetNativeLibraryName("base_profiler_test_support_library"));
    NativeLibraryLoadError load_error;
    *library = LoadNativeLibrary(other_library_path, &load_error);
    ASSERT_TRUE(*library) << "error loading " << other_library_path.value()
                          << ": " << load_error.ToString();
  };

  NativeLibrary library = nullptr;
  load(&library);
  return library;
}

// Unloads |library| and returns when it has completed unloading. Unloading a
// library is asynchronous on Windows, so simply calling UnloadNativeLibrary()
// is insufficient to ensure it's been unloaded.
void SynchronousUnloadNativeLibrary(NativeLibrary library) {
  UnloadNativeLibrary(library);
#if defined(OS_WIN)
  // NativeLibrary is a typedef for HMODULE, which is actually the base address
  // of the module.
  uintptr_t module_base_address = reinterpret_cast<uintptr_t>(library);
  HMODULE module_handle;
  // Keep trying to get the module handle until the call fails.
  while (::GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                             GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                             reinterpret_cast<LPCTSTR>(module_base_address),
                             &module_handle) ||
         ::GetLastError() != ERROR_MOD_NOT_FOUND) {
    PlatformThread::Sleep(TimeDelta::FromMilliseconds(1));
  }
#elif defined(OS_MACOSX)
// Unloading a library on the Mac is synchronous.
#else
  NOTIMPLEMENTED();
#endif
}

// Called on the profiler thread when complete, to collect profiles.
void SaveProfiles(CallStackProfiles* profiles,
                  CallStackProfiles pending_profiles) {
  *profiles = std::move(pending_profiles);
}

// Called on the profiler thread when complete. Collects profiles produced by
// the profiler, and signals an event to allow the main thread to know that that
// the profiler is done.
void SaveProfilesAndSignalEvent(CallStackProfiles* profiles,
                                WaitableEvent* event,
                                CallStackProfiles pending_profiles) {
  *profiles = std::move(pending_profiles);
  event->Signal();
}

// Executes the function with the target thread running and executing within
// SignalAndWaitUntilSignaled(). Performs all necessary target thread startup
// and shutdown work before and afterward.
template <class Function>
void WithTargetThread(Function function,
                      const StackConfiguration& stack_config) {
  TargetThread target_thread(stack_config);
  PlatformThreadHandle target_thread_handle;
  EXPECT_TRUE(PlatformThread::Create(0, &target_thread, &target_thread_handle));

  target_thread.WaitForThreadStart();

  function(target_thread.id());

  target_thread.SignalThreadToFinish();

  PlatformThread::Join(target_thread_handle);
}

template <class Function>
void WithTargetThread(Function function) {
  WithTargetThread(function, StackConfiguration(StackConfiguration::NORMAL));
}

struct TestProfilerInfo {
  TestProfilerInfo(PlatformThreadId thread_id,
                   const SamplingParams& params,
                   NativeStackSamplerTestDelegate* delegate = nullptr)
      : completed(WaitableEvent::ResetPolicy::MANUAL,
                  WaitableEvent::InitialState::NOT_SIGNALED),
        profiler(thread_id,
                 params,
                 Bind(&SaveProfilesAndSignalEvent,
                      Unretained(&profiles),
                      Unretained(&completed)),
                 delegate) {}

  // The order here is important to ensure objects being referenced don't get
  // destructed until after the objects referencing them.
  CallStackProfiles profiles;
  WaitableEvent completed;
  StackSamplingProfiler profiler;

 private:
  DISALLOW_COPY_AND_ASSIGN(TestProfilerInfo);
};

// Creates multiple profilers based on a vector of parameters.
std::vector<std::unique_ptr<TestProfilerInfo>> CreateProfilers(
    PlatformThreadId target_thread_id,
    const std::vector<SamplingParams>& params) {
  DCHECK(!params.empty());

  std::vector<std::unique_ptr<TestProfilerInfo>> profilers;
  for (size_t i = 0; i < params.size(); ++i) {
    profilers.push_back(
        std::make_unique<TestProfilerInfo>(target_thread_id, params[i]));
  }

  return profilers;
}

// Captures profiles as specified by |params| on the TargetThread, and returns
// them in |profiles|. Waits up to |profiler_wait_time| for the profiler to
// complete.
void CaptureProfiles(const SamplingParams& params, TimeDelta profiler_wait_time,
                     CallStackProfiles* profiles) {
  WithTargetThread([&params, profiles,
                    profiler_wait_time](PlatformThreadId target_thread_id) {
    TestProfilerInfo info(target_thread_id, params);
    info.profiler.Start();
    info.completed.TimedWait(profiler_wait_time);
    info.profiler.Stop();
    info.completed.Wait();

    *profiles = std::move(info.profiles);
  });
}

// Waits for one of multiple samplings to complete.
size_t WaitForSamplingComplete(
    const std::vector<std::unique_ptr<TestProfilerInfo>>& infos) {
  // Map unique_ptrs to something that WaitMany can accept.
  std::vector<WaitableEvent*> sampling_completed_rawptrs(infos.size());
  std::transform(infos.begin(), infos.end(), sampling_completed_rawptrs.begin(),
                 [](const std::unique_ptr<TestProfilerInfo>& info) {
                   return &info.get()->completed;
                 });
  // Wait for one profiler to finish.
  return WaitableEvent::WaitMany(sampling_completed_rawptrs.data(),
                                 sampling_completed_rawptrs.size());
}

// If this executable was linked with /INCREMENTAL (the default for non-official
// debug and release builds on Windows), function addresses do not correspond to
// function code itself, but instead to instructions in the Incremental Link
// Table that jump to the functions. Checks for a jump instruction and if
// present does a little decompilation to find the function's actual starting
// address.
const void* MaybeFixupFunctionAddressForILT(const void* function_address) {
#if defined(_WIN64)
  const unsigned char* opcode =
      reinterpret_cast<const unsigned char*>(function_address);
  if (*opcode == 0xe9) {
    // This is a relative jump instruction. Assume we're in the ILT and compute
    // the function start address from the instruction offset.
    const int32_t* offset = reinterpret_cast<const int32_t*>(opcode + 1);
    const unsigned char* next_instruction =
        reinterpret_cast<const unsigned char*>(offset + 1);
    return next_instruction + *offset;
  }
#endif
  return function_address;
}

// Searches through the frames in |sample|, returning an iterator to the first
// frame that has an instruction pointer within |target_function|. Returns
// sample.end() if no such frames are found.
Frames::const_iterator FindFirstFrameWithinFunction(
    const Sample& sample,
    TargetFunction target_function) {
  uintptr_t function_start = reinterpret_cast<uintptr_t>(
      MaybeFixupFunctionAddressForILT(reinterpret_cast<const void*>(
          target_function)));
  uintptr_t function_end =
      reinterpret_cast<uintptr_t>(target_function(nullptr, nullptr, nullptr));
  for (auto it = sample.frames.begin(); it != sample.frames.end(); ++it) {
    if ((it->instruction_pointer >= function_start) &&
        (it->instruction_pointer <= function_end))
      return it;
  }
  return sample.frames.end();
}

// Formats a sample into a string that can be output for test diagnostics.
std::string FormatSampleForDiagnosticOutput(
    const Sample& sample,
    const std::vector<Module>& modules) {
  std::string output;
  for (const Frame& frame : sample.frames) {
    output += StringPrintf(
        "0x%p %s\n", reinterpret_cast<const void*>(frame.instruction_pointer),
        modules[frame.module_index].filename.AsUTF8Unsafe().c_str());
  }
  return output;
}

// Returns a duration that is longer than the test timeout. We would use
// TimeDelta::Max() but https://crbug.com/465948.
TimeDelta AVeryLongTimeDelta() { return TimeDelta::FromDays(1); }

// Tests the scenario where the library is unloaded after copying the stack, but
// before walking it. If |wait_until_unloaded| is true, ensures that the
// asynchronous library loading has completed before walking the stack. If
// false, the unloading may still be occurring during the stack walk.
void TestLibraryUnload(bool wait_until_unloaded) {
  // Test delegate that supports intervening between the copying of the stack
  // and the walking of the stack.
  class StackCopiedSignaler : public NativeStackSamplerTestDelegate {
   public:
    StackCopiedSignaler(WaitableEvent* stack_copied,
                        WaitableEvent* start_stack_walk,
                        bool wait_to_walk_stack)
        : stack_copied_(stack_copied),
          start_stack_walk_(start_stack_walk),
          wait_to_walk_stack_(wait_to_walk_stack) {}

    void OnPreStackWalk() override {
      stack_copied_->Signal();
      if (wait_to_walk_stack_)
        start_stack_walk_->Wait();
    }

   private:
    WaitableEvent* const stack_copied_;
    WaitableEvent* const start_stack_walk_;
    const bool wait_to_walk_stack_;
  };

  SamplingParams params;
  params.sampling_interval = TimeDelta::FromMilliseconds(0);
  params.samples_per_burst = 1;

  NativeLibrary other_library = LoadOtherLibrary();
  TargetThread target_thread(StackConfiguration(
      StackConfiguration::WITH_OTHER_LIBRARY,
      other_library));

  PlatformThreadHandle target_thread_handle;
  EXPECT_TRUE(PlatformThread::Create(0, &target_thread, &target_thread_handle));

  target_thread.WaitForThreadStart();

  WaitableEvent sampling_thread_completed(
      WaitableEvent::ResetPolicy::MANUAL,
      WaitableEvent::InitialState::NOT_SIGNALED);
  std::vector<CallStackProfile> profiles;
  const StackSamplingProfiler::CompletedCallback callback =
      Bind(&SaveProfilesAndSignalEvent, Unretained(&profiles),
           Unretained(&sampling_thread_completed));
  WaitableEvent stack_copied(WaitableEvent::ResetPolicy::MANUAL,
                             WaitableEvent::InitialState::NOT_SIGNALED);
  WaitableEvent start_stack_walk(WaitableEvent::ResetPolicy::MANUAL,
                                 WaitableEvent::InitialState::NOT_SIGNALED);
  StackCopiedSignaler test_delegate(&stack_copied, &start_stack_walk,
                                    wait_until_unloaded);
  StackSamplingProfiler profiler(target_thread.id(), params, callback,
                                 &test_delegate);

  profiler.Start();

  // Wait for the stack to be copied and the target thread to be resumed.
  stack_copied.Wait();

  // Cause the target thread to finish, so that it's no longer executing code in
  // the library we're about to unload.
  target_thread.SignalThreadToFinish();
  PlatformThread::Join(target_thread_handle);

  // Unload the library now that it's not being used.
  if (wait_until_unloaded)
    SynchronousUnloadNativeLibrary(other_library);
  else
    UnloadNativeLibrary(other_library);

  // Let the stack walk commence after unloading the library, if we're waiting
  // on that event.
  start_stack_walk.Signal();

  // Wait for the sampling thread to complete and fill out |profiles|.
  sampling_thread_completed.Wait();

  // Look up the sample.
  ASSERT_EQ(1u, profiles.size());
  const CallStackProfile& profile = profiles[0];
  ASSERT_EQ(1u, profile.samples.size());
  const Sample& sample = profile.samples[0];

  // Check that the stack contains a frame for
  // TargetThread::SignalAndWaitUntilSignaled().
  Frames::const_iterator end_frame = FindFirstFrameWithinFunction(
      sample, &TargetThread::SignalAndWaitUntilSignaled);
  ASSERT_TRUE(end_frame != sample.frames.end())
      << "Function at "
      << MaybeFixupFunctionAddressForILT(reinterpret_cast<const void*>(
             &TargetThread::SignalAndWaitUntilSignaled))
      << " was not found in stack:\n"
      << FormatSampleForDiagnosticOutput(sample, profile.modules);

  if (wait_until_unloaded) {
    // The stack should look like this, resulting one frame after
    // SignalAndWaitUntilSignaled. The frame in the now-unloaded library is not
    // recorded since we can't get module information.
    //
    // ... WaitableEvent and system frames ...
    // TargetThread::SignalAndWaitUntilSignaled
    // TargetThread::OtherLibraryCallback
    EXPECT_EQ(2, sample.frames.end() - end_frame)
        << "Stack:\n"
        << FormatSampleForDiagnosticOutput(sample, profile.modules);
  } else {
    // We didn't wait for the asynchronous unloading to complete, so the results
    // are non-deterministic: if the library finished unloading we should have
    // the same stack as |wait_until_unloaded|, if not we should have the full
    // stack. The important thing is that we should not crash.

    if (sample.frames.end() - end_frame == 2) {
      // This is the same case as |wait_until_unloaded|.
      return;
    }

    // Check that the stack contains a frame for
    // TargetThread::CallThroughOtherLibrary().
    Frames::const_iterator other_library_frame = FindFirstFrameWithinFunction(
        sample, &TargetThread::CallThroughOtherLibrary);
    ASSERT_TRUE(other_library_frame != sample.frames.end())
        << "Function at "
        << MaybeFixupFunctionAddressForILT(reinterpret_cast<const void*>(
               &TargetThread::CallThroughOtherLibrary))
        << " was not found in stack:\n"
        << FormatSampleForDiagnosticOutput(sample, profile.modules);

    // The stack should look like this, resulting in three frames between
    // SignalAndWaitUntilSignaled and CallThroughOtherLibrary:
    //
    // ... WaitableEvent and system frames ...
    // TargetThread::SignalAndWaitUntilSignaled
    // TargetThread::OtherLibraryCallback
    // InvokeCallbackFunction (in other library)
    // TargetThread::CallThroughOtherLibrary
    EXPECT_EQ(3, other_library_frame - end_frame)
        << "Stack:\n"
        << FormatSampleForDiagnosticOutput(sample, profile.modules);
  }
}

// Provide a suitable (and clean) environment for the tests below. All tests
// must use this class to ensure that proper clean-up is done and thus be
// usable in a later test.
class StackSamplingProfilerTest : public testing::Test {
 public:
  void SetUp() override {
    // The idle-shutdown time is too long for convenient (and accurate) testing.
    // That behavior is checked instead by artificially triggering it through
    // the TestAPI.
    StackSamplingProfiler::TestAPI::DisableIdleShutdown();
  }

  void TearDown() override {
    // Be a good citizen and clean up after ourselves. This also re-enables the
    // idle-shutdown behavior.
    StackSamplingProfiler::TestAPI::Reset();
  }
};

}  // namespace

// Checks that the basic expected information is present in a sampled call stack
// profile.
// macOS ASAN is not yet supported - crbug.com/718628.
#if !(defined(ADDRESS_SANITIZER) && defined(OS_MACOSX))
#define MAYBE_Basic Basic
#else
#define MAYBE_Basic DISABLED_Basic
#endif
PROFILER_TEST_F(StackSamplingProfilerTest, MAYBE_Basic) {
  SamplingParams params;
  params.sampling_interval = TimeDelta::FromMilliseconds(0);
  params.samples_per_burst = 1;

  std::vector<CallStackProfile> profiles;
  CaptureProfiles(params, AVeryLongTimeDelta(), &profiles);

  // Check that the profile and samples sizes are correct, and the module
  // indices are in range.
  ASSERT_EQ(1u, profiles.size());
  const CallStackProfile& profile = profiles[0];
  ASSERT_EQ(1u, profile.samples.size());
  EXPECT_EQ(params.sampling_interval, profile.sampling_period);
  const Sample& sample = profile.samples[0];
  EXPECT_EQ(0u, sample.process_milestones);
  for (const auto& frame : sample.frames) {
    ASSERT_GE(frame.module_index, 0u);
    ASSERT_LT(frame.module_index, profile.modules.size());
  }

  // Check that the stack contains a frame for
  // TargetThread::SignalAndWaitUntilSignaled() and that the frame has this
  // executable's module.
  Frames::const_iterator loc = FindFirstFrameWithinFunction(
      sample, &TargetThread::SignalAndWaitUntilSignaled);
  ASSERT_TRUE(loc != sample.frames.end())
      << "Function at "
      << MaybeFixupFunctionAddressForILT(reinterpret_cast<const void*>(
             &TargetThread::SignalAndWaitUntilSignaled))
      << " was not found in stack:\n"
      << FormatSampleForDiagnosticOutput(sample, profile.modules);
  FilePath executable_path;
  EXPECT_TRUE(PathService::Get(FILE_EXE, &executable_path));
  EXPECT_EQ(executable_path,
            MakeAbsoluteFilePath(profile.modules[loc->module_index].filename));
}

// Checks that annotations are recorded in samples.
PROFILER_TEST_F(StackSamplingProfilerTest, Annotations) {
  SamplingParams params;
  params.sampling_interval = TimeDelta::FromMilliseconds(0);
  params.samples_per_burst = 1;

  // Check that a run picks up annotations.
  StackSamplingProfiler::SetProcessMilestone(1);
  std::vector<CallStackProfile> profiles1;
  CaptureProfiles(params, AVeryLongTimeDelta(), &profiles1);
  ASSERT_EQ(1u, profiles1.size());
  const CallStackProfile& profile1 = profiles1[0];
  ASSERT_EQ(1u, profile1.samples.size());
  const Sample& sample1 = profile1.samples[0];
  EXPECT_EQ(1u << 1, sample1.process_milestones);

  // Run it a second time but with changed annotations. These annotations
  // should appear in the first acquired sample.
  StackSamplingProfiler::SetProcessMilestone(2);
  std::vector<CallStackProfile> profiles2;
  CaptureProfiles(params, AVeryLongTimeDelta(), &profiles2);
  ASSERT_EQ(1u, profiles2.size());
  const CallStackProfile& profile2 = profiles2[0];
  ASSERT_EQ(1u, profile2.samples.size());
  const Sample& sample2 = profile2.samples[0];
  EXPECT_EQ(sample1.process_milestones | (1u << 2), sample2.process_milestones);
}

// Checks that the profiler handles stacks containing dynamically-allocated
// stack memory.
// macOS ASAN is not yet supported - crbug.com/718628.
#if !(defined(ADDRESS_SANITIZER) && defined(OS_MACOSX))
#define MAYBE_Alloca Alloca
#else
#define MAYBE_Alloca DISABLED_Alloca
#endif
PROFILER_TEST_F(StackSamplingProfilerTest, MAYBE_Alloca) {
  SamplingParams params;
  params.sampling_interval = TimeDelta::FromMilliseconds(0);
  params.samples_per_burst = 1;

  std::vector<CallStackProfile> profiles;
  WithTargetThread(
      [&params, &profiles](PlatformThreadId target_thread_id) {
        WaitableEvent sampling_thread_completed(
            WaitableEvent::ResetPolicy::MANUAL,
            WaitableEvent::InitialState::NOT_SIGNALED);
        const StackSamplingProfiler::CompletedCallback callback =
            Bind(&SaveProfilesAndSignalEvent, Unretained(&profiles),
                 Unretained(&sampling_thread_completed));
        StackSamplingProfiler profiler(target_thread_id, params, callback);
        profiler.Start();
        sampling_thread_completed.Wait();
      },
      StackConfiguration(StackConfiguration::WITH_ALLOCA));

  // Look up the sample.
  ASSERT_EQ(1u, profiles.size());
  const CallStackProfile& profile = profiles[0];
  ASSERT_EQ(1u, profile.samples.size());
  const Sample& sample = profile.samples[0];

  // Check that the stack contains a frame for
  // TargetThread::SignalAndWaitUntilSignaled().
  Frames::const_iterator end_frame = FindFirstFrameWithinFunction(
      sample, &TargetThread::SignalAndWaitUntilSignaled);
  ASSERT_TRUE(end_frame != sample.frames.end())
      << "Function at "
      << MaybeFixupFunctionAddressForILT(reinterpret_cast<const void*>(
             &TargetThread::SignalAndWaitUntilSignaled))
      << " was not found in stack:\n"
      << FormatSampleForDiagnosticOutput(sample, profile.modules);

  // Check that the stack contains a frame for TargetThread::CallWithAlloca().
  Frames::const_iterator alloca_frame =
      FindFirstFrameWithinFunction(sample, &TargetThread::CallWithAlloca);
  ASSERT_TRUE(alloca_frame != sample.frames.end())
      << "Function at "
      << MaybeFixupFunctionAddressForILT(
             reinterpret_cast<const void*>(&TargetThread::CallWithAlloca))
      << " was not found in stack:\n"
      << FormatSampleForDiagnosticOutput(sample, profile.modules);

  // These frames should be adjacent on the stack.
  EXPECT_EQ(1, alloca_frame - end_frame)
      << "Stack:\n"
      << FormatSampleForDiagnosticOutput(sample, profile.modules);
}

// Checks that the expected number of profiles and samples are present in the
// call stack profiles produced.
PROFILER_TEST_F(StackSamplingProfilerTest, MultipleProfilesAndSamples) {
  SamplingParams params;
  params.burst_interval = params.sampling_interval =
      TimeDelta::FromMilliseconds(0);
  params.bursts = 2;
  params.samples_per_burst = 3;

  std::vector<CallStackProfile> profiles;
  CaptureProfiles(params, AVeryLongTimeDelta(), &profiles);

  ASSERT_EQ(2u, profiles.size());
  EXPECT_EQ(3u, profiles[0].samples.size());
  EXPECT_EQ(3u, profiles[1].samples.size());
}

// Checks that a profiler can stop/destruct without ever having started.
PROFILER_TEST_F(StackSamplingProfilerTest, StopWithoutStarting) {
  WithTargetThread([](PlatformThreadId target_thread_id) {
    SamplingParams params;
    params.sampling_interval = TimeDelta::FromMilliseconds(0);
    params.samples_per_burst = 1;

    CallStackProfiles profiles;
    WaitableEvent sampling_completed(WaitableEvent::ResetPolicy::MANUAL,
                                     WaitableEvent::InitialState::NOT_SIGNALED);
    const StackSamplingProfiler::CompletedCallback callback =
        Bind(&SaveProfilesAndSignalEvent, Unretained(&profiles),
             Unretained(&sampling_completed));
    StackSamplingProfiler profiler(target_thread_id, params, callback);

    profiler.Stop();  // Constructed but never started.
    EXPECT_FALSE(sampling_completed.IsSignaled());
  });
}

// Checks that its okay to stop a profiler before it finishes even when the
// sampling thread continues to run.
PROFILER_TEST_F(StackSamplingProfilerTest, StopSafely) {
  // Test delegate that counts samples.
  class SampleRecordedCounter : public NativeStackSamplerTestDelegate {
   public:
    SampleRecordedCounter() = default;

    void OnPreStackWalk() override {
      AutoLock lock(lock_);
      ++count_;
    }

    size_t Get() {
      AutoLock lock(lock_);
      return count_;
    }

   private:
    Lock lock_;
    size_t count_ = 0;
  };

  WithTargetThread([](PlatformThreadId target_thread_id) {
    SamplingParams params[2];

    // Providing an initial delay makes it more likely that both will be
    // scheduled before either starts to run. Once started, samples will
    // run ordered by their scheduled, interleaved times regardless of
    // whatever interval the thread wakes up.
    params[0].initial_delay = TimeDelta::FromMilliseconds(10);
    params[0].sampling_interval = TimeDelta::FromMilliseconds(1);
    params[0].samples_per_burst = 100000;

    params[1].initial_delay = TimeDelta::FromMilliseconds(10);
    params[1].sampling_interval = TimeDelta::FromMilliseconds(1);
    params[1].samples_per_burst = 100000;

    SampleRecordedCounter samples_recorded[arraysize(params)];

    TestProfilerInfo profiler_info0(target_thread_id, params[0],
                                    &samples_recorded[0]);
    TestProfilerInfo profiler_info1(target_thread_id, params[1],
                                    &samples_recorded[1]);

    profiler_info0.profiler.Start();
    profiler_info1.profiler.Start();

    // Wait for both to start accumulating samples. Using a WaitableEvent is
    // possible but gets complicated later on because there's no way of knowing
    // if 0 or 1 additional sample will be taken after Stop() and thus no way
    // of knowing how many Wait() calls to make on it.
    while (samples_recorded[0].Get() == 0 || samples_recorded[1].Get() == 0)
      PlatformThread::Sleep(TimeDelta::FromMilliseconds(1));

    // Ensure that the first sampler can be safely stopped while the second
    // continues to run. The stopped first profiler will still have a
    // PerformCollectionTask pending that will do nothing when executed because
    // the collection will have been removed by Stop().
    profiler_info0.profiler.Stop();
    profiler_info0.completed.Wait();
    size_t count0 = samples_recorded[0].Get();
    size_t count1 = samples_recorded[1].Get();

    // Waiting for the second sampler to collect a couple samples ensures that
    // the pending PerformCollectionTask for the first has executed because
    // tasks are always ordered by their next scheduled time.
    while (samples_recorded[1].Get() < count1 + 2)
      PlatformThread::Sleep(TimeDelta::FromMilliseconds(1));

    // Ensure that the first profiler didn't do anything since it was stopped.
    EXPECT_EQ(count0, samples_recorded[0].Get());
  });
}

// Checks that no call stack profiles are captured if the profiling is stopped
// during the initial delay.
PROFILER_TEST_F(StackSamplingProfilerTest, StopDuringInitialDelay) {
  SamplingParams params;
  params.initial_delay = TimeDelta::FromSeconds(60);

  std::vector<CallStackProfile> profiles;
  CaptureProfiles(params, TimeDelta::FromMilliseconds(0), &profiles);

  EXPECT_TRUE(profiles.empty());
}

// Checks that the single completed call stack profile is captured if the
// profiling is stopped between bursts.
PROFILER_TEST_F(StackSamplingProfilerTest, StopDuringInterBurstInterval) {
  SamplingParams params;
  params.sampling_interval = TimeDelta::FromMilliseconds(0);
  params.burst_interval = TimeDelta::FromSeconds(60);
  params.bursts = 2;
  params.samples_per_burst = 1;

  std::vector<CallStackProfile> profiles;
  CaptureProfiles(params, TimeDelta::FromMilliseconds(50), &profiles);

  ASSERT_EQ(1u, profiles.size());
  EXPECT_EQ(1u, profiles[0].samples.size());
}

// Checks that tasks can be stopped before completion and incomplete call stack
// profiles are captured.
PROFILER_TEST_F(StackSamplingProfilerTest, StopDuringInterSampleInterval) {
  // Test delegate that counts samples.
  class SampleRecordedEvent : public NativeStackSamplerTestDelegate {
   public:
    SampleRecordedEvent()
        : sample_recorded_(WaitableEvent::ResetPolicy::MANUAL,
                           WaitableEvent::InitialState::NOT_SIGNALED) {}

    void OnPreStackWalk() override { sample_recorded_.Signal(); }

    void WaitForSample() { sample_recorded_.Wait(); }

   private:
    WaitableEvent sample_recorded_;
  };

  WithTargetThread([](PlatformThreadId target_thread_id) {
    SamplingParams params;

    params.sampling_interval = AVeryLongTimeDelta();
    params.samples_per_burst = 2;

    SampleRecordedEvent samples_recorded;
    TestProfilerInfo profiler_info(target_thread_id, params, &samples_recorded);

    profiler_info.profiler.Start();

    // Wait for profiler to start accumulating samples.
    samples_recorded.WaitForSample();

    // Ensure that it can stop safely.
    profiler_info.profiler.Stop();
    profiler_info.completed.Wait();

    ASSERT_EQ(1u, profiler_info.profiles.size());
    EXPECT_EQ(1u, profiler_info.profiles[0].samples.size());
  });
}

// Checks that we can destroy the profiler while profiling.
PROFILER_TEST_F(StackSamplingProfilerTest, DestroyProfilerWhileProfiling) {
  SamplingParams params;
  params.sampling_interval = TimeDelta::FromMilliseconds(10);

  CallStackProfiles profiles;
  WithTargetThread([&params, &profiles](PlatformThreadId target_thread_id) {
    std::unique_ptr<StackSamplingProfiler> profiler;
    profiler.reset(new StackSamplingProfiler(
        target_thread_id, params, Bind(&SaveProfiles, Unretained(&profiles))));
    profiler->Start();
    profiler.reset();

    // Wait longer than a sample interval to catch any use-after-free actions by
    // the profiler thread.
    PlatformThread::Sleep(TimeDelta::FromMilliseconds(50));
  });
}

// Checks that the same profiler may be run multiple times.
PROFILER_TEST_F(StackSamplingProfilerTest, CanRunMultipleTimes) {
  WithTargetThread([](PlatformThreadId target_thread_id) {
    SamplingParams params;
    params.sampling_interval = TimeDelta::FromMilliseconds(0);
    params.samples_per_burst = 1;

    CallStackProfiles profiles;
    WaitableEvent sampling_completed(WaitableEvent::ResetPolicy::MANUAL,
                                     WaitableEvent::InitialState::NOT_SIGNALED);
    const StackSamplingProfiler::CompletedCallback callback =
        Bind(&SaveProfilesAndSignalEvent, Unretained(&profiles),
             Unretained(&sampling_completed));
    StackSamplingProfiler profiler(target_thread_id, params, callback);

    // Just start and stop to execute code paths.
    profiler.Start();
    profiler.Stop();
    sampling_completed.Wait();

    // Ensure a second request will run and not block.
    sampling_completed.Reset();
    profiles.clear();
    profiler.Start();
    sampling_completed.Wait();
    profiler.Stop();
    ASSERT_EQ(1u, profiles.size());
  });
}

// Checks that the different profilers may be run.
PROFILER_TEST_F(StackSamplingProfilerTest, CanRunMultipleProfilers) {
  SamplingParams params;
  params.sampling_interval = TimeDelta::FromMilliseconds(0);
  params.samples_per_burst = 1;

  std::vector<CallStackProfile> profiles;
  CaptureProfiles(params, AVeryLongTimeDelta(), &profiles);
  ASSERT_EQ(1u, profiles.size());

  profiles.clear();
  CaptureProfiles(params, AVeryLongTimeDelta(), &profiles);
  ASSERT_EQ(1u, profiles.size());
}

// Checks that a sampler can be started while another is running.
PROFILER_TEST_F(StackSamplingProfilerTest, MultipleStart) {
  WithTargetThread([](PlatformThreadId target_thread_id) {
    std::vector<SamplingParams> params(2);

    params[0].initial_delay = AVeryLongTimeDelta();
    params[0].samples_per_burst = 1;

    params[1].sampling_interval = TimeDelta::FromMilliseconds(1);
    params[1].samples_per_burst = 1;

    std::vector<std::unique_ptr<TestProfilerInfo>> profiler_infos =
        CreateProfilers(target_thread_id, params);

    profiler_infos[0]->profiler.Start();
    profiler_infos[1]->profiler.Start();
    profiler_infos[1]->completed.Wait();
    EXPECT_EQ(1u, profiler_infos[1]->profiles.size());
  });
}

// Checks that the sampling thread can shut down.
PROFILER_TEST_F(StackSamplingProfilerTest, SamplerIdleShutdown) {
  SamplingParams params;
  params.sampling_interval = TimeDelta::FromMilliseconds(0);
  params.samples_per_burst = 1;

  std::vector<CallStackProfile> profiles;
  CaptureProfiles(params, AVeryLongTimeDelta(), &profiles);
  ASSERT_EQ(1u, profiles.size());

  // Capture thread should still be running at this point.
  ASSERT_TRUE(StackSamplingProfiler::TestAPI::IsSamplingThreadRunning());

  // Initiate an "idle" shutdown and ensure it happens. Idle-shutdown was
  // disabled by the test fixture so the test will fail due to a timeout if
  // it does not exit.
  StackSamplingProfiler::TestAPI::PerformSamplingThreadIdleShutdown(false);

  // While the shutdown has been initiated, the actual exit of the thread still
  // happens asynchronously. Watch until the thread actually exits. This test
  // will time-out in the case of failure.
  while (StackSamplingProfiler::TestAPI::IsSamplingThreadRunning())
    PlatformThread::Sleep(base::TimeDelta::FromMilliseconds(1));
}

// Checks that additional requests will restart a stopped profiler.
PROFILER_TEST_F(StackSamplingProfilerTest,
                WillRestartSamplerAfterIdleShutdown) {
  SamplingParams params;
  params.sampling_interval = TimeDelta::FromMilliseconds(0);
  params.samples_per_burst = 1;

  std::vector<CallStackProfile> profiles;
  CaptureProfiles(params, AVeryLongTimeDelta(), &profiles);
  ASSERT_EQ(1u, profiles.size());

  // Capture thread should still be running at this point.
  ASSERT_TRUE(StackSamplingProfiler::TestAPI::IsSamplingThreadRunning());

  // Post a ShutdownTask on the sampling thread which, when executed, will
  // mark the thread as EXITING and begin shut down of the thread.
  StackSamplingProfiler::TestAPI::PerformSamplingThreadIdleShutdown(false);

  // Ensure another capture will start the sampling thread and run.
  profiles.clear();
  CaptureProfiles(params, AVeryLongTimeDelta(), &profiles);
  ASSERT_EQ(1u, profiles.size());
  EXPECT_TRUE(StackSamplingProfiler::TestAPI::IsSamplingThreadRunning());
}

// Checks that it's safe to stop a task after it's completed and the sampling
// thread has shut-down for being idle.
PROFILER_TEST_F(StackSamplingProfilerTest, StopAfterIdleShutdown) {
  WithTargetThread([](PlatformThreadId target_thread_id) {
    SamplingParams params;

    params.sampling_interval = TimeDelta::FromMilliseconds(1);
    params.samples_per_burst = 1;

    TestProfilerInfo profiler_info(target_thread_id, params);

    profiler_info.profiler.Start();
    profiler_info.completed.Wait();

    // Capture thread should still be running at this point.
    ASSERT_TRUE(StackSamplingProfiler::TestAPI::IsSamplingThreadRunning());

    // Perform an idle shutdown.
    StackSamplingProfiler::TestAPI::PerformSamplingThreadIdleShutdown(false);

    // Stop should be safe though its impossible to know at this moment if the
    // sampling thread has completely exited or will just "stop soon".
    profiler_info.profiler.Stop();
  });
}

// Checks that profilers can run both before and after the sampling thread has
// started.
PROFILER_TEST_F(StackSamplingProfilerTest,
                ProfileBeforeAndAfterSamplingThreadRunning) {
  WithTargetThread([](PlatformThreadId target_thread_id) {
    std::vector<SamplingParams> params(2);

    params[0].initial_delay = AVeryLongTimeDelta();
    params[0].sampling_interval = TimeDelta::FromMilliseconds(1);
    params[0].samples_per_burst = 1;

    params[1].initial_delay = TimeDelta::FromMilliseconds(0);
    params[1].sampling_interval = TimeDelta::FromMilliseconds(1);
    params[1].samples_per_burst = 1;

    std::vector<std::unique_ptr<TestProfilerInfo>> profiler_infos =
        CreateProfilers(target_thread_id, params);

    // First profiler is started when there has never been a sampling thread.
    EXPECT_FALSE(StackSamplingProfiler::TestAPI::IsSamplingThreadRunning());
    profiler_infos[0]->profiler.Start();
    // Second profiler is started when sampling thread is already running.
    EXPECT_TRUE(StackSamplingProfiler::TestAPI::IsSamplingThreadRunning());
    profiler_infos[1]->profiler.Start();

    // Only the second profiler should finish before test times out.
    size_t completed_profiler = WaitForSamplingComplete(profiler_infos);
    EXPECT_EQ(1U, completed_profiler);
  });
}

// Checks that an idle-shutdown task will abort if a new profiler starts
// between when it was posted and when it runs.
PROFILER_TEST_F(StackSamplingProfilerTest, IdleShutdownAbort) {
  WithTargetThread([](PlatformThreadId target_thread_id) {
    SamplingParams params;

    params.sampling_interval = TimeDelta::FromMilliseconds(1);
    params.samples_per_burst = 1;

    TestProfilerInfo profiler_info(target_thread_id, params);

    profiler_info.profiler.Start();
    profiler_info.completed.Wait();
    EXPECT_EQ(1u, profiler_info.profiles.size());

    // Perform an idle shutdown but simulate that a new capture is started
    // before it can actually run.
    StackSamplingProfiler::TestAPI::PerformSamplingThreadIdleShutdown(true);

    // Though the shutdown-task has been executed, any actual exit of the
    // thread is asynchronous so there is no way to detect that *didn't* exit
    // except to wait a reasonable amount of time and then check. Since the
    // thread was just running ("perform" blocked until it was), it should
    // finish almost immediately and without any waiting for tasks or events.
    PlatformThread::Sleep(base::TimeDelta::FromMilliseconds(200));
    EXPECT_TRUE(StackSamplingProfiler::TestAPI::IsSamplingThreadRunning());

    // Ensure that it's still possible to run another sampler.
    TestProfilerInfo another_info(target_thread_id, params);
    another_info.profiler.Start();
    another_info.completed.Wait();
    EXPECT_EQ(1u, another_info.profiles.size());
  });
}

// Checks that synchronized multiple sampling requests execute in parallel.
PROFILER_TEST_F(StackSamplingProfilerTest, ConcurrentProfiling_InSync) {
  WithTargetThread([](PlatformThreadId target_thread_id) {
    std::vector<SamplingParams> params(2);

    // Providing an initial delay makes it more likely that both will be
    // scheduled before either starts to run. Once started, samples will
    // run ordered by their scheduled, interleaved times regardless of
    // whatever interval the thread wakes up. Thus, total execution time
    // will be 10ms (delay) + 10x1ms (sampling) + 1/2 timer minimum interval.
    params[0].initial_delay = TimeDelta::FromMilliseconds(10);
    params[0].sampling_interval = TimeDelta::FromMilliseconds(1);
    params[0].samples_per_burst = 9;

    params[1].initial_delay = TimeDelta::FromMilliseconds(11);
    params[1].sampling_interval = TimeDelta::FromMilliseconds(1);
    params[1].samples_per_burst = 8;

    std::vector<std::unique_ptr<TestProfilerInfo>> profiler_infos =
        CreateProfilers(target_thread_id, params);

    profiler_infos[0]->profiler.Start();
    profiler_infos[1]->profiler.Start();

    // Wait for one profiler to finish.
    size_t completed_profiler = WaitForSamplingComplete(profiler_infos);
    ASSERT_EQ(1u, profiler_infos[completed_profiler]->profiles.size());

    size_t other_profiler = 1 - completed_profiler;
    // Wait for the other profiler to finish.
    profiler_infos[other_profiler]->completed.Wait();
    ASSERT_EQ(1u, profiler_infos[other_profiler]->profiles.size());

    // Ensure each got the correct number of samples.
    EXPECT_EQ(9u, profiler_infos[0]->profiles[0].samples.size());
    EXPECT_EQ(8u, profiler_infos[1]->profiles[0].samples.size());
  });
}

// Checks that several mixed sampling requests execute in parallel.
PROFILER_TEST_F(StackSamplingProfilerTest, ConcurrentProfiling_Mixed) {
  WithTargetThread([](PlatformThreadId target_thread_id) {
    std::vector<SamplingParams> params(3);

    params[0].initial_delay = TimeDelta::FromMilliseconds(8);
    params[0].sampling_interval = TimeDelta::FromMilliseconds(4);
    params[0].samples_per_burst = 10;

    params[1].initial_delay = TimeDelta::FromMilliseconds(9);
    params[1].sampling_interval = TimeDelta::FromMilliseconds(3);
    params[1].samples_per_burst = 10;

    params[2].initial_delay = TimeDelta::FromMilliseconds(10);
    params[2].sampling_interval = TimeDelta::FromMilliseconds(2);
    params[2].samples_per_burst = 10;

    std::vector<std::unique_ptr<TestProfilerInfo>> profiler_infos =
        CreateProfilers(target_thread_id, params);

    for (size_t i = 0; i < profiler_infos.size(); ++i)
      profiler_infos[i]->profiler.Start();

    // Wait for one profiler to finish.
    size_t completed_profiler = WaitForSamplingComplete(profiler_infos);
    EXPECT_EQ(1u, profiler_infos[completed_profiler]->profiles.size());
    // Stop and destroy all profilers, always in the same order. Don't crash.
    for (size_t i = 0; i < profiler_infos.size(); ++i)
      profiler_infos[i]->profiler.Stop();
    for (size_t i = 0; i < profiler_infos.size(); ++i)
      profiler_infos[i].reset();
  });
}

// Checks that a stack that runs through another library produces a stack with
// the expected functions.
// macOS ASAN is not yet supported - crbug.com/718628.
#if !(defined(ADDRESS_SANITIZER) && defined(OS_MACOSX))
#define MAYBE_OtherLibrary OtherLibrary
#else
#define MAYBE_OtherLibrary DISABLED_OtherLibrary
#endif
PROFILER_TEST_F(StackSamplingProfilerTest, MAYBE_OtherLibrary) {
  SamplingParams params;
  params.sampling_interval = TimeDelta::FromMilliseconds(0);
  params.samples_per_burst = 1;

  std::vector<CallStackProfile> profiles;
  {
    ScopedNativeLibrary other_library(LoadOtherLibrary());
    WithTargetThread(
        [&params, &profiles](PlatformThreadId target_thread_id) {
          WaitableEvent sampling_thread_completed(
              WaitableEvent::ResetPolicy::MANUAL,
              WaitableEvent::InitialState::NOT_SIGNALED);
          const StackSamplingProfiler::CompletedCallback callback =
              Bind(&SaveProfilesAndSignalEvent, Unretained(&profiles),
                   Unretained(&sampling_thread_completed));
          StackSamplingProfiler profiler(target_thread_id, params, callback);
          profiler.Start();
          sampling_thread_completed.Wait();
        },
        StackConfiguration(StackConfiguration::WITH_OTHER_LIBRARY,
                           other_library.get()));
  }

  // Look up the sample.
  ASSERT_EQ(1u, profiles.size());
  const CallStackProfile& profile = profiles[0];
  ASSERT_EQ(1u, profile.samples.size());
  const Sample& sample = profile.samples[0];

  // Check that the stack contains a frame for
  // TargetThread::CallThroughOtherLibrary().
  Frames::const_iterator other_library_frame = FindFirstFrameWithinFunction(
      sample, &TargetThread::CallThroughOtherLibrary);
  ASSERT_TRUE(other_library_frame != sample.frames.end())
      << "Function at "
      << MaybeFixupFunctionAddressForILT(reinterpret_cast<const void*>(
             &TargetThread::CallThroughOtherLibrary))
      << " was not found in stack:\n"
      << FormatSampleForDiagnosticOutput(sample, profile.modules);

  // Check that the stack contains a frame for
  // TargetThread::SignalAndWaitUntilSignaled().
  Frames::const_iterator end_frame = FindFirstFrameWithinFunction(
      sample, &TargetThread::SignalAndWaitUntilSignaled);
  ASSERT_TRUE(end_frame != sample.frames.end())
      << "Function at "
      << MaybeFixupFunctionAddressForILT(reinterpret_cast<const void*>(
             &TargetThread::SignalAndWaitUntilSignaled))
      << " was not found in stack:\n"
      << FormatSampleForDiagnosticOutput(sample, profile.modules);

  // The stack should look like this, resulting in three frames between
  // SignalAndWaitUntilSignaled and CallThroughOtherLibrary:
  //
  // ... WaitableEvent and system frames ...
  // TargetThread::SignalAndWaitUntilSignaled
  // TargetThread::OtherLibraryCallback
  // InvokeCallbackFunction (in other library)
  // TargetThread::CallThroughOtherLibrary
  EXPECT_EQ(3, other_library_frame - end_frame)
      << "Stack:\n" << FormatSampleForDiagnosticOutput(sample, profile.modules);
}

// Checks that a stack that runs through a library that is unloading produces a
// stack, and doesn't crash.
// Unloading is synchronous on the Mac, so this test is inapplicable.
#if !defined(OS_MACOSX)
#define MAYBE_UnloadingLibrary UnloadingLibrary
#else
#define MAYBE_UnloadingLibrary DISABLED_UnloadingLibrary
#endif
PROFILER_TEST_F(StackSamplingProfilerTest, MAYBE_UnloadingLibrary) {
  TestLibraryUnload(false);
}

// Checks that a stack that runs through a library that has been unloaded
// produces a stack, and doesn't crash.
// macOS ASAN is not yet supported - crbug.com/718628.
#if !(defined(ADDRESS_SANITIZER) && defined(OS_MACOSX))
#define MAYBE_UnloadedLibrary UnloadedLibrary
#else
#define MAYBE_UnloadedLibrary DISABLED_UnloadedLibrary
#endif
PROFILER_TEST_F(StackSamplingProfilerTest, MAYBE_UnloadedLibrary) {
  TestLibraryUnload(true);
}

// Checks that different threads can be sampled in parallel.
PROFILER_TEST_F(StackSamplingProfilerTest, MultipleSampledThreads) {
  // Create target threads. The extra parethesis around the StackConfiguration
  // call are to avoid the most-vexing-parse problem.
  TargetThread target_thread1((StackConfiguration(StackConfiguration::NORMAL)));
  TargetThread target_thread2((StackConfiguration(StackConfiguration::NORMAL)));
  PlatformThreadHandle target_thread_handle1, target_thread_handle2;
  EXPECT_TRUE(
      PlatformThread::Create(0, &target_thread1, &target_thread_handle1));
  EXPECT_TRUE(
      PlatformThread::Create(0, &target_thread2, &target_thread_handle2));
  target_thread1.WaitForThreadStart();
  target_thread2.WaitForThreadStart();

  // Providing an initial delay makes it more likely that both will be
  // scheduled before either starts to run. Once started, samples will
  // run ordered by their scheduled, interleaved times regardless of
  // whatever interval the thread wakes up.
  SamplingParams params1, params2;
  params1.initial_delay = TimeDelta::FromMilliseconds(10);
  params1.sampling_interval = TimeDelta::FromMilliseconds(1);
  params1.samples_per_burst = 9;
  params2.initial_delay = TimeDelta::FromMilliseconds(10);
  params2.sampling_interval = TimeDelta::FromMilliseconds(1);
  params2.samples_per_burst = 8;

  std::vector<CallStackProfile> profiles1, profiles2;

  WaitableEvent sampling_thread_completed1(
      WaitableEvent::ResetPolicy::MANUAL,
      WaitableEvent::InitialState::NOT_SIGNALED);
  const StackSamplingProfiler::CompletedCallback callback1 =
      Bind(&SaveProfilesAndSignalEvent, Unretained(&profiles1),
           Unretained(&sampling_thread_completed1));
  StackSamplingProfiler profiler1(target_thread1.id(), params1, callback1);

  WaitableEvent sampling_thread_completed2(
      WaitableEvent::ResetPolicy::MANUAL,
      WaitableEvent::InitialState::NOT_SIGNALED);
  const StackSamplingProfiler::CompletedCallback callback2 =
      Bind(&SaveProfilesAndSignalEvent, Unretained(&profiles2),
           Unretained(&sampling_thread_completed2));
  StackSamplingProfiler profiler2(target_thread2.id(), params2, callback2);

  // Finally the real work.
  profiler1.Start();
  profiler2.Start();
  sampling_thread_completed1.Wait();
  sampling_thread_completed2.Wait();
  ASSERT_EQ(1u, profiles1.size());
  EXPECT_EQ(9u, profiles1[0].samples.size());
  ASSERT_EQ(1u, profiles2.size());
  EXPECT_EQ(8u, profiles2[0].samples.size());

  target_thread1.SignalThreadToFinish();
  target_thread2.SignalThreadToFinish();
  PlatformThread::Join(target_thread_handle1);
  PlatformThread::Join(target_thread_handle2);
}

// A simple thread that runs a profiler on another thread.
class ProfilerThread : public SimpleThread {
 public:
  ProfilerThread(const std::string& name,
                 PlatformThreadId thread_id,
                 const SamplingParams& params)
      : SimpleThread(name, Options()),
        run_(WaitableEvent::ResetPolicy::MANUAL,
             WaitableEvent::InitialState::NOT_SIGNALED),
        completed_(WaitableEvent::ResetPolicy::MANUAL,
                   WaitableEvent::InitialState::NOT_SIGNALED),
        profiler_(thread_id,
                  params,
                  Bind(&SaveProfilesAndSignalEvent,
                       Unretained(&profiles_),
                       Unretained(&completed_))) {}

  void Run() override {
    run_.Wait();
    profiler_.Start();
  }

  void Go() { run_.Signal(); }

  void Wait() { completed_.Wait(); }

  CallStackProfiles& profiles() { return profiles_; }

 private:
  WaitableEvent run_;

  CallStackProfiles profiles_;
  WaitableEvent completed_;
  StackSamplingProfiler profiler_;
};

// Checks that different threads can run samplers in parallel.
PROFILER_TEST_F(StackSamplingProfilerTest, MultipleProfilerThreads) {
  WithTargetThread([](PlatformThreadId target_thread_id) {
    // Providing an initial delay makes it more likely that both will be
    // scheduled before either starts to run. Once started, samples will
    // run ordered by their scheduled, interleaved times regardless of
    // whatever interval the thread wakes up.
    SamplingParams params1, params2;
    params1.initial_delay = TimeDelta::FromMilliseconds(10);
    params1.sampling_interval = TimeDelta::FromMilliseconds(1);
    params1.samples_per_burst = 9;
    params2.initial_delay = TimeDelta::FromMilliseconds(10);
    params2.sampling_interval = TimeDelta::FromMilliseconds(1);
    params2.samples_per_burst = 8;

    // Start the profiler threads and give them a moment to get going.
    ProfilerThread profiler_thread1("profiler1", target_thread_id, params1);
    ProfilerThread profiler_thread2("profiler2", target_thread_id, params2);
    profiler_thread1.Start();
    profiler_thread2.Start();
    PlatformThread::Sleep(base::TimeDelta::FromMilliseconds(10));

    // This will (approximately) synchronize the two threads.
    profiler_thread1.Go();
    profiler_thread2.Go();

    // Wait for them both to finish and validate collection.
    profiler_thread1.Wait();
    profiler_thread2.Wait();
    ASSERT_EQ(1u, profiler_thread1.profiles().size());
    EXPECT_EQ(9u, profiler_thread1.profiles()[0].samples.size());
    ASSERT_EQ(1u, profiler_thread2.profiles().size());
    EXPECT_EQ(8u, profiler_thread2.profiles()[0].samples.size());

    profiler_thread1.Join();
    profiler_thread2.Join();
  });
}

}  // namespace base
