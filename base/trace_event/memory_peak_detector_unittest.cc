// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/trace_event/memory_peak_detector.h"

#include <memory>

#include "base/bind.h"
#include "base/logging.h"
#include "base/run_loop.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/platform_thread.h"
#include "base/threading/thread.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/trace_event/memory_dump_provider.h"
#include "base/trace_event/memory_dump_provider_info.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;

namespace base {
namespace trace_event {

namespace {

const TimeDelta kMs = TimeDelta::FromMilliseconds(1);
const MemoryPeakDetector::Config kConfigNoCallbacks(
    1 /* polling_interval_ms */,
    60000 /* min_time_between_peaks_ms */,
    false /* enable_verbose_poll_tracing */
    );

class MockMemoryDumpProvider : public MemoryDumpProvider {
 public:
  bool OnMemoryDump(const MemoryDumpArgs&, ProcessMemoryDump*) override {
    NOTREACHED();
    return true;
  }

  MOCK_METHOD1(PollFastMemoryTotal, void(uint64_t*));
};

// Wrapper to use gmock on a callback.
struct OnPeakDetectedWrapper {
  MOCK_METHOD0(OnPeak, void());
};

}  // namespace

class MemoryPeakDetectorTest : public testing::Test {
 public:
  struct FriendDeleter {
    void operator()(MemoryPeakDetector* inst) { delete inst; }
  };

  MemoryPeakDetectorTest() : testing::Test() {}
  static const uint64_t kSlidingWindowNumSamples =
      MemoryPeakDetector::kSlidingWindowNumSamples;

  std::unique_ptr<MemoryPeakDetector, FriendDeleter> NewInstance() {
    return std::unique_ptr<MemoryPeakDetector, FriendDeleter>(
        new MemoryPeakDetector());
  }

  void RestartThreadAndReinitializePeakDetector() {
    bg_thread_.reset(new Thread("Peak Detector Test Thread"));
    bg_thread_->Start();
    peak_detector_ = NewInstance();
    peak_detector_->Setup(
        Bind(&MemoryPeakDetectorTest::MockGetDumpProviders, Unretained(this)),
        bg_thread_->task_runner(),
        Bind(&OnPeakDetectedWrapper::OnPeak, Unretained(&on_peak_callback_)));
  }

  void SetUp() override {
    get_mdp_call_count_ = 0;
    RestartThreadAndReinitializePeakDetector();
  }

  void TearDown() override {
    peak_detector_->TearDown();
    bg_thread_->FlushForTesting();
    EXPECT_EQ(MemoryPeakDetector::NOT_INITIALIZED, GetPeakDetectorState());
    bg_thread_.reset();
    dump_providers_.clear();
  }

  // Calls MemoryPeakDetector::state_for_testing() on the bg thread and returns
  // the result on the current thread.
  MemoryPeakDetector::State GetPeakDetectorState() {
    WaitableEvent evt(WaitableEvent::ResetPolicy::MANUAL,
                      WaitableEvent::InitialState::NOT_SIGNALED);
    MemoryPeakDetector::State res = MemoryPeakDetector::NOT_INITIALIZED;
    auto get_fn = [](MemoryPeakDetector* peak_detector, WaitableEvent* evt,
                     MemoryPeakDetector::State* res) {
      *res = peak_detector->state_for_testing();
      evt->Signal();
    };
    bg_thread_->task_runner()->PostTask(
        FROM_HERE, BindOnce(get_fn, Unretained(&*peak_detector_),
                            Unretained(&evt), Unretained(&res)));
    evt.Wait();
    return res;
  }

  // Calls MemoryPeakDetector::poll_tasks_count_for_testing() on the bg thread
  // and returns the result on the current thread.
  uint32_t GetNumPollingTasksRan() {
    uint32_t res = 0;
    auto get_fn = [](MemoryPeakDetector* peak_detector, WaitableEvent* evt,
                     uint32_t* res) {
      *res = peak_detector->poll_tasks_count_for_testing();
      evt->Signal();
    };

    WaitableEvent evt(WaitableEvent::ResetPolicy::MANUAL,
                      WaitableEvent::InitialState::NOT_SIGNALED);
    bg_thread_->task_runner()->PostTask(
        FROM_HERE, BindOnce(get_fn, Unretained(&*peak_detector_),
                            Unretained(&evt), Unretained(&res)));
    evt.Wait();
    return res;
  }

  // Runs the peak detector with a mock MDP with the given
  // |config|. The mock MDP will invoke the |poll_function| on any call to
  // PollFastMemoryTotal(), until |num_samples| have been polled.
  // It returns the number of peaks detected.
  uint32_t RunWithCustomPollFunction(
      MemoryPeakDetector::Config config,
      uint32_t num_samples,
      RepeatingCallback<uint64_t(uint32_t)> poll_function) {
    WaitableEvent evt(WaitableEvent::ResetPolicy::MANUAL,
                      WaitableEvent::InitialState::NOT_SIGNALED);
    scoped_refptr<MemoryDumpProviderInfo> mdp = CreateMockDumpProvider();
    dump_providers_.push_back(mdp);
    uint32_t cur_sample_idx = 0;
    EXPECT_CALL(GetMockMDP(mdp), PollFastMemoryTotal(_))
        .WillRepeatedly(Invoke(
            [&cur_sample_idx, &evt, poll_function, num_samples](uint64_t* mem) {
              if (cur_sample_idx >= num_samples) {
                *mem = 1;
                evt.Signal();
              } else {
                *mem = poll_function.Run(cur_sample_idx++);
              }
            }));

    uint32_t num_peaks = 0;
    EXPECT_CALL(on_peak_callback_, OnPeak())
        .WillRepeatedly(Invoke([&num_peaks] { num_peaks++; }));
    peak_detector_->Start(config);
    evt.Wait();  // Wait for |num_samples| invocations of PollFastMemoryTotal().
    peak_detector_->Stop();
    EXPECT_EQ(num_samples, cur_sample_idx);
    EXPECT_EQ(MemoryPeakDetector::DISABLED, GetPeakDetectorState());
    return num_peaks;
  }

  // Called on the |bg_thread_|.
  void MockGetDumpProviders(MemoryPeakDetector::DumpProvidersList* mdps) {
    get_mdp_call_count_++;
    *mdps = dump_providers_;
  }

  uint32_t GetNumGetDumpProvidersCalls() {
    bg_thread_->FlushForTesting();
    return get_mdp_call_count_;
  }

  scoped_refptr<MemoryDumpProviderInfo> CreateMockDumpProvider() {
    std::unique_ptr<MockMemoryDumpProvider> mdp(new MockMemoryDumpProvider());
    MemoryDumpProvider::Options opt;
    opt.is_fast_polling_supported = true;
    scoped_refptr<MemoryDumpProviderInfo> mdp_info(new MemoryDumpProviderInfo(
        mdp.get(), "Mock MDP", nullptr, opt,
        false /* whitelisted_for_background_mode */));

    // The |mdp| instance will be destroyed together with the |mdp_info|.
    mdp_info->owned_dump_provider = std::move(mdp);
    return mdp_info;
  }

  static MockMemoryDumpProvider& GetMockMDP(
      const scoped_refptr<MemoryDumpProviderInfo>& mdp_info) {
    return *static_cast<MockMemoryDumpProvider*>(mdp_info->dump_provider);
  }

  static uint64_t PollFunctionThatCausesPeakViaStdDev(uint32_t sample_idx) {
    // Start with a baseline of 50 MB.
    if (sample_idx < kSlidingWindowNumSamples)
      return 50000 + (sample_idx % 3) * 100;

    // Then 10 samples around 80 MB
    if (sample_idx < 10 + kSlidingWindowNumSamples)
      return 80000 + (sample_idx % 3) * 200;

    // Than back to 60 MB.
    if (sample_idx < 2 * kSlidingWindowNumSamples)
      return 60000 + (sample_idx % 3) * 100;

    // Then 20 samples around 120 MB.
    if (sample_idx < 20 + 2 * kSlidingWindowNumSamples)
      return 120000 + (sample_idx % 3) * 200;

    // Then back to idle to around 50 MB until the end.
    return 50000 + (sample_idx % 3) * 100;
  }

 protected:
  MemoryPeakDetector::DumpProvidersList dump_providers_;
  uint32_t get_mdp_call_count_;
  std::unique_ptr<MemoryPeakDetector, FriendDeleter> peak_detector_;
  std::unique_ptr<Thread> bg_thread_;
  OnPeakDetectedWrapper on_peak_callback_;
};

const uint64_t MemoryPeakDetectorTest::kSlidingWindowNumSamples;

TEST_F(MemoryPeakDetectorTest, GetDumpProvidersFunctionCalled) {
  EXPECT_EQ(MemoryPeakDetector::DISABLED, GetPeakDetectorState());
  peak_detector_->Start(kConfigNoCallbacks);
  EXPECT_EQ(1u, GetNumGetDumpProvidersCalls());
  EXPECT_EQ(MemoryPeakDetector::ENABLED, GetPeakDetectorState());

  peak_detector_->Stop();
  EXPECT_EQ(MemoryPeakDetector::DISABLED, GetPeakDetectorState());
  EXPECT_EQ(0u, GetNumPollingTasksRan());
}

TEST_F(MemoryPeakDetectorTest, ThrottleAndNotifyBeforeInitialize) {
  peak_detector_->TearDown();

  WaitableEvent evt(WaitableEvent::ResetPolicy::MANUAL,
                    WaitableEvent::InitialState::NOT_SIGNALED);
  scoped_refptr<MemoryDumpProviderInfo> mdp = CreateMockDumpProvider();
  EXPECT_CALL(GetMockMDP(mdp), PollFastMemoryTotal(_))
      .WillRepeatedly(Invoke([&evt](uint64_t*) { evt.Signal(); }));
  dump_providers_.push_back(mdp);
  peak_detector_->Throttle();
  peak_detector_->NotifyMemoryDumpProvidersChanged();
  EXPECT_EQ(MemoryPeakDetector::NOT_INITIALIZED, GetPeakDetectorState());
  RestartThreadAndReinitializePeakDetector();

  peak_detector_->Start(kConfigNoCallbacks);
  EXPECT_EQ(MemoryPeakDetector::RUNNING, GetPeakDetectorState());
  evt.Wait();  // Wait for a PollFastMemoryTotal() call.

  peak_detector_->Stop();
  EXPECT_EQ(MemoryPeakDetector::DISABLED, GetPeakDetectorState());
  EXPECT_EQ(1u, GetNumGetDumpProvidersCalls());
  EXPECT_GE(GetNumPollingTasksRan(), 1u);
}

TEST_F(MemoryPeakDetectorTest, DoubleStop) {
  peak_detector_->Start(kConfigNoCallbacks);
  EXPECT_EQ(MemoryPeakDetector::ENABLED, GetPeakDetectorState());

  peak_detector_->Stop();
  EXPECT_EQ(MemoryPeakDetector::DISABLED, GetPeakDetectorState());

  peak_detector_->Stop();
  EXPECT_EQ(MemoryPeakDetector::DISABLED, GetPeakDetectorState());

  EXPECT_EQ(1u, GetNumGetDumpProvidersCalls());
  EXPECT_EQ(0u, GetNumPollingTasksRan());
}

TEST_F(MemoryPeakDetectorTest, OneDumpProviderRegisteredBeforeStart) {
  WaitableEvent evt(WaitableEvent::ResetPolicy::MANUAL,
                    WaitableEvent::InitialState::NOT_SIGNALED);
  scoped_refptr<MemoryDumpProviderInfo> mdp = CreateMockDumpProvider();
  EXPECT_CALL(GetMockMDP(mdp), PollFastMemoryTotal(_))
      .WillRepeatedly(Invoke([&evt](uint64_t*) { evt.Signal(); }));
  dump_providers_.push_back(mdp);

  peak_detector_->Start(kConfigNoCallbacks);
  evt.Wait();  // Signaled when PollFastMemoryTotal() is called on the MockMDP.
  EXPECT_EQ(MemoryPeakDetector::RUNNING, GetPeakDetectorState());

  peak_detector_->Stop();
  EXPECT_EQ(MemoryPeakDetector::DISABLED, GetPeakDetectorState());
  EXPECT_EQ(1u, GetNumGetDumpProvidersCalls());
  EXPECT_GT(GetNumPollingTasksRan(), 0u);
}

TEST_F(MemoryPeakDetectorTest, ReInitializeAndRebindToNewThread) {
  WaitableEvent evt(WaitableEvent::ResetPolicy::MANUAL,
                    WaitableEvent::InitialState::NOT_SIGNALED);
  scoped_refptr<MemoryDumpProviderInfo> mdp = CreateMockDumpProvider();
  EXPECT_CALL(GetMockMDP(mdp), PollFastMemoryTotal(_))
      .WillRepeatedly(Invoke([&evt](uint64_t*) { evt.Signal(); }));
  dump_providers_.push_back(mdp);

  for (int i = 0; i < 5; ++i) {
    evt.Reset();
    peak_detector_->Start(kConfigNoCallbacks);
    evt.Wait();  // Wait for a PollFastMemoryTotal() call.
    // Check that calling TearDown implicitly does a Stop().
    peak_detector_->TearDown();

    // Reinitialize and re-bind to a new task runner.
    RestartThreadAndReinitializePeakDetector();
  }
}

TEST_F(MemoryPeakDetectorTest, OneDumpProviderRegisteredOutOfBand) {
  peak_detector_->Start(kConfigNoCallbacks);
  EXPECT_EQ(MemoryPeakDetector::ENABLED, GetPeakDetectorState());
  EXPECT_EQ(1u, GetNumGetDumpProvidersCalls());

  // Check that no poll tasks are posted before any dump provider is registered.
  PlatformThread::Sleep(5 * kConfigNoCallbacks.polling_interval_ms * kMs);
  EXPECT_EQ(0u, GetNumPollingTasksRan());

  // Registed the MDP After Start() has been issued and expect that the
  // PeakDetector transitions ENABLED -> RUNNING on the next
  // NotifyMemoryDumpProvidersChanged() call.
  WaitableEvent evt(WaitableEvent::ResetPolicy::MANUAL,
                    WaitableEvent::InitialState::NOT_SIGNALED);
  scoped_refptr<MemoryDumpProviderInfo> mdp = CreateMockDumpProvider();
  EXPECT_CALL(GetMockMDP(mdp), PollFastMemoryTotal(_))
      .WillRepeatedly(Invoke([&evt](uint64_t*) { evt.Signal(); }));
  dump_providers_.push_back(mdp);
  peak_detector_->NotifyMemoryDumpProvidersChanged();

  evt.Wait();  // Signaled when PollFastMemoryTotal() is called on the MockMDP.
  EXPECT_EQ(MemoryPeakDetector::RUNNING, GetPeakDetectorState());
  EXPECT_EQ(2u, GetNumGetDumpProvidersCalls());

  // Now simulate the unregisration and expect that the PeakDetector transitions
  // back to ENABLED.
  dump_providers_.clear();
  peak_detector_->NotifyMemoryDumpProvidersChanged();
  EXPECT_EQ(MemoryPeakDetector::ENABLED, GetPeakDetectorState());
  EXPECT_EQ(3u, GetNumGetDumpProvidersCalls());
  uint32_t num_poll_tasks = GetNumPollingTasksRan();
  EXPECT_GT(num_poll_tasks, 0u);

  // At this point, no more polling tasks should be posted.
  PlatformThread::Sleep(5 * kConfigNoCallbacks.polling_interval_ms * kMs);
  peak_detector_->Stop();
  EXPECT_EQ(MemoryPeakDetector::DISABLED, GetPeakDetectorState());
  EXPECT_EQ(num_poll_tasks, GetNumPollingTasksRan());
}

// Test that a sequence of Start()/Stop() back-to-back doesn't end up creating
// several outstanding timer tasks and instead respects the polling_interval_ms.
TEST_F(MemoryPeakDetectorTest, StartStopQuickly) {
  WaitableEvent evt(WaitableEvent::ResetPolicy::MANUAL,
                    WaitableEvent::InitialState::NOT_SIGNALED);
  scoped_refptr<MemoryDumpProviderInfo> mdp = CreateMockDumpProvider();
  dump_providers_.push_back(mdp);
  const uint32_t kNumPolls = 20;
  uint32_t polls_done = 0;
  EXPECT_CALL(GetMockMDP(mdp), PollFastMemoryTotal(_))
      .WillRepeatedly(Invoke([&polls_done, &evt, kNumPolls](uint64_t*) {
        if (++polls_done == kNumPolls)
          evt.Signal();
      }));

  const TimeTicks tstart = TimeTicks::Now();
  for (int i = 0; i < 5; i++) {
    peak_detector_->Start(kConfigNoCallbacks);
    peak_detector_->Stop();
  }

  bg_thread_->task_runner()->PostTask(
      FROM_HERE, base::BindOnce([](uint32_t* polls_done) { *polls_done = 0; },
                                &polls_done));

  peak_detector_->Start(kConfigNoCallbacks);
  EXPECT_EQ(MemoryPeakDetector::RUNNING, GetPeakDetectorState());
  evt.Wait();  // Wait for kNumPolls.
  const double time_ms = (TimeTicks::Now() - tstart).InMillisecondsF();

  EXPECT_GE(time_ms, (kNumPolls - 1) * kConfigNoCallbacks.polling_interval_ms);
  peak_detector_->Stop();
}

TEST_F(MemoryPeakDetectorTest, RegisterAndUnregisterTwoDumpProviders) {
  WaitableEvent evt1(WaitableEvent::ResetPolicy::MANUAL,
                     WaitableEvent::InitialState::NOT_SIGNALED);
  WaitableEvent evt2(WaitableEvent::ResetPolicy::MANUAL,
                     WaitableEvent::InitialState::NOT_SIGNALED);
  scoped_refptr<MemoryDumpProviderInfo> mdp1 = CreateMockDumpProvider();
  scoped_refptr<MemoryDumpProviderInfo> mdp2 = CreateMockDumpProvider();
  EXPECT_CALL(GetMockMDP(mdp1), PollFastMemoryTotal(_))
      .WillRepeatedly(Invoke([&evt1](uint64_t*) { evt1.Signal(); }));
  EXPECT_CALL(GetMockMDP(mdp2), PollFastMemoryTotal(_))
      .WillRepeatedly(Invoke([&evt2](uint64_t*) { evt2.Signal(); }));

  // Register only one MDP and start the detector.
  dump_providers_.push_back(mdp1);
  peak_detector_->Start(kConfigNoCallbacks);
  EXPECT_EQ(MemoryPeakDetector::RUNNING, GetPeakDetectorState());

  // Wait for one poll task and then register also the other one.
  evt1.Wait();
  dump_providers_.push_back(mdp2);
  peak_detector_->NotifyMemoryDumpProvidersChanged();
  evt2.Wait();
  EXPECT_EQ(MemoryPeakDetector::RUNNING, GetPeakDetectorState());

  // Now unregister the first MDP and check that everything is still running.
  dump_providers_.erase(dump_providers_.begin());
  peak_detector_->NotifyMemoryDumpProvidersChanged();
  EXPECT_EQ(MemoryPeakDetector::RUNNING, GetPeakDetectorState());

  // Now unregister both and check that the detector goes to idle.
  dump_providers_.clear();
  peak_detector_->NotifyMemoryDumpProvidersChanged();
  EXPECT_EQ(MemoryPeakDetector::ENABLED, GetPeakDetectorState());

  // Now re-register both and check that the detector re-activates posting
  // new polling tasks.
  uint32_t num_poll_tasks = GetNumPollingTasksRan();
  evt1.Reset();
  evt2.Reset();
  dump_providers_.push_back(mdp1);
  dump_providers_.push_back(mdp2);
  peak_detector_->NotifyMemoryDumpProvidersChanged();
  evt1.Wait();
  evt2.Wait();
  EXPECT_EQ(MemoryPeakDetector::RUNNING, GetPeakDetectorState());
  EXPECT_GT(GetNumPollingTasksRan(), num_poll_tasks);

  // Stop everything, tear down the MDPs, restart the detector and check that
  // it detector doesn't accidentally try to re-access them.
  peak_detector_->Stop();
  EXPECT_EQ(MemoryPeakDetector::DISABLED, GetPeakDetectorState());
  dump_providers_.clear();
  mdp1 = nullptr;
  mdp2 = nullptr;

  num_poll_tasks = GetNumPollingTasksRan();
  peak_detector_->Start(kConfigNoCallbacks);
  EXPECT_EQ(MemoryPeakDetector::ENABLED, GetPeakDetectorState());
  PlatformThread::Sleep(5 * kConfigNoCallbacks.polling_interval_ms * kMs);

  peak_detector_->Stop();
  EXPECT_EQ(MemoryPeakDetector::DISABLED, GetPeakDetectorState());
  EXPECT_EQ(num_poll_tasks, GetNumPollingTasksRan());

  EXPECT_EQ(6u, GetNumGetDumpProvidersCalls());
}

// Tests the behavior of the static threshold detector, which is supposed to
// detect a peak whenever an increase >= threshold is detected.
TEST_F(MemoryPeakDetectorTest, StaticThreshold) {
  const uint32_t kNumSamples = 2 * kSlidingWindowNumSamples;
  constexpr uint32_t kNumSamplesPerStep = 10;
  constexpr uint64_t kThreshold = 1000000;
  peak_detector_->SetStaticThresholdForTesting(kThreshold);
  const MemoryPeakDetector::Config kConfig(
      1 /* polling_interval_ms */, 0 /* min_time_between_peaks_ms */,
      false /* enable_verbose_poll_tracing */
      );

  // The mocked PollFastMemoryTotal() will return a step function,
  // e.g. (1, 1, 1, 5, 5, 5, ...) where the steps are 2x threshold, in order to
  // trigger only the static threshold logic.
  auto poll_fn = Bind(
      [](const uint32_t kNumSamplesPerStep, const uint64_t kThreshold,
         uint32_t sample_idx) -> uint64_t {
        return (1 + sample_idx / kNumSamplesPerStep) * 2 * kThreshold;
      },
      kNumSamplesPerStep, kThreshold);
  uint32_t num_peaks = RunWithCustomPollFunction(kConfig, kNumSamples, poll_fn);
  EXPECT_EQ(kNumSamples / kNumSamplesPerStep - 1, num_peaks);
}

// Checks the throttling logic of Config's |min_time_between_peaks_ms|.
TEST_F(MemoryPeakDetectorTest, PeakCallbackThrottling) {
  const size_t kNumSamples = 2 * kSlidingWindowNumSamples;
  constexpr uint64_t kThreshold = 1000000;
  peak_detector_->SetStaticThresholdForTesting(kThreshold);
  const MemoryPeakDetector::Config kConfig(
      1 /* polling_interval_ms */, 4 /* min_time_between_peaks_ms */,
      false /* enable_verbose_poll_tracing */
      );

  // Each mock value returned is N * 2 * threshold, so all of them would be
  // eligible to be a peak if throttling wasn't enabled.
  auto poll_fn = Bind(
      [](uint64_t kThreshold, uint32_t sample_idx) -> uint64_t {
        return (sample_idx + 1) * 2 * kThreshold;
      },
      kThreshold);
  uint32_t num_peaks = RunWithCustomPollFunction(kConfig, kNumSamples, poll_fn);
  const uint32_t kExpectedThrottlingRate =
      kConfig.min_time_between_peaks_ms / kConfig.polling_interval_ms;
  EXPECT_LT(num_peaks, kNumSamples / kExpectedThrottlingRate);
}

TEST_F(MemoryPeakDetectorTest, StdDev) {
  // Set the threshold to some arbitrarily high value, so that the static
  // threshold logic is not hit in this test.
  constexpr uint64_t kThreshold = 1024 * 1024 * 1024;
  peak_detector_->SetStaticThresholdForTesting(kThreshold);
  const size_t kNumSamples = 3 * kSlidingWindowNumSamples;
  const MemoryPeakDetector::Config kConfig(
      1 /* polling_interval_ms */, 0 /* min_time_between_peaks_ms */,
      false /* enable_verbose_poll_tracing */
      );

  auto poll_fn = Bind(&PollFunctionThatCausesPeakViaStdDev);
  uint32_t num_peaks = RunWithCustomPollFunction(kConfig, kNumSamples, poll_fn);
  EXPECT_EQ(2u, num_peaks);  // 80 MB, 120 MB.
}

// Tests that Throttle() actually holds back peak notifications.
TEST_F(MemoryPeakDetectorTest, Throttle) {
  constexpr uint64_t kThreshold = 1024 * 1024 * 1024;
  const uint32_t kNumSamples = 3 * kSlidingWindowNumSamples;
  peak_detector_->SetStaticThresholdForTesting(kThreshold);
  const MemoryPeakDetector::Config kConfig(
      1 /* polling_interval_ms */, 0 /* min_time_between_peaks_ms */,
      false /* enable_verbose_poll_tracing */
      );

  auto poll_fn = Bind(
      [](MemoryPeakDetector* peak_detector, uint32_t sample_idx) -> uint64_t {
        if (sample_idx % 20 == 20 - 1)
          peak_detector->Throttle();
        return PollFunctionThatCausesPeakViaStdDev(sample_idx);
      },
      Unretained(&*peak_detector_));
  uint32_t num_peaks = RunWithCustomPollFunction(kConfig, kNumSamples, poll_fn);
  EXPECT_EQ(0u, num_peaks);
}

// Tests that the windows stddev state is not carried over through
// Stop() -> Start() sequences.
TEST_F(MemoryPeakDetectorTest, RestartClearsState) {
  constexpr uint64_t kThreshold = 1024 * 1024 * 1024;
  peak_detector_->SetStaticThresholdForTesting(kThreshold);
  const size_t kNumSamples = 3 * kSlidingWindowNumSamples;
  const MemoryPeakDetector::Config kConfig(
      1 /* polling_interval_ms */, 0 /* min_time_between_peaks_ms */,
      false /* enable_verbose_poll_tracing */
      );
  auto poll_fn = Bind(
      [](MemoryPeakDetector* peak_detector,
         const uint32_t kSlidingWindowNumSamples,
         MemoryPeakDetector::Config kConfig, uint32_t sample_idx) -> uint64_t {
        if (sample_idx % kSlidingWindowNumSamples ==
            kSlidingWindowNumSamples - 1) {
          peak_detector->Stop();
          peak_detector->Start(kConfig);
        }
        return PollFunctionThatCausesPeakViaStdDev(sample_idx);
      },
      Unretained(&*peak_detector_), kSlidingWindowNumSamples, kConfig);
  uint32_t num_peaks = RunWithCustomPollFunction(kConfig, kNumSamples, poll_fn);
  EXPECT_EQ(0u, num_peaks);
}

}  // namespace trace_event
}  // namespace base
