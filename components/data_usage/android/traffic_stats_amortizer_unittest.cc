// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/data_usage/android/traffic_stats_amortizer.h"

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/metrics/histogram_base.h"
#include "base/run_loop.h"
#include "base/test/histogram_tester.h"
#include "base/test/simple_test_tick_clock.h"
#include "base/time/tick_clock.h"
#include "base/time/time.h"
#include "base/timer/mock_timer.h"
#include "base/timer/timer.h"
#include "components/data_usage/core/data_use.h"
#include "components/data_usage/core/data_use_amortizer.h"
#include "net/base/network_change_notifier.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace data_usage {
namespace android {

namespace {

// The delay between receiving DataUse and querying TrafficStats byte counts for
// amortization.
const base::TimeDelta kTrafficStatsQueryDelay =
    base::TimeDelta::FromMilliseconds(50);

// The longest amount of time that an amortization run can be delayed for.
const base::TimeDelta kMaxAmortizationDelay =
    base::TimeDelta::FromMilliseconds(200);

// The maximum allowed size of the DataUse buffer.
const size_t kMaxDataUseBufferSize = 8;

const char kPreAmortizationTxHistogram[] =
    "TrafficStatsAmortizer.PreAmortizationRunDataUseBytes.Tx";
const char kPreAmortizationRxHistogram[] =
    "TrafficStatsAmortizer.PreAmortizationRunDataUseBytes.Rx";
const char kPostAmortizationTxHistogram[] =
    "TrafficStatsAmortizer.PostAmortizationRunDataUseBytes.Tx";
const char kPostAmortizationRxHistogram[] =
    "TrafficStatsAmortizer.PostAmortizationRunDataUseBytes.Rx";
const char kAmortizationDelayHistogram[] =
    "TrafficStatsAmortizer.AmortizationDelay";
const char kBufferSizeOnFlushHistogram[] =
    "TrafficStatsAmortizer.BufferSizeOnFlush";
const char kConcurrentTabs[] = "TrafficStatsAmortizer.ConcurrentTabs";

// The maximum sample value that can be recorded in a histogram.
const base::HistogramBase::Sample kMaxRecordableSample =
    base::HistogramBase::kSampleType_MAX - 1;

// Converts a |delay| into a histogram sample of the number of milliseconds in
// the delay.
base::HistogramBase::Sample GetDelaySample(const base::TimeDelta& delay) {
  return static_cast<base::HistogramBase::Sample>(delay.InMilliseconds());
}

// Synthesizes a fake std::unique_ptr<DataUse> with the given |url|, |tab_id|,
// |tx_bytes| and |rx_bytes|, using arbitrary values for all other fields.
std::unique_ptr<DataUse> CreateDataUseWithURLAndTab(const GURL& url,
                                                    SessionID tab_id,
                                                    int64_t tx_bytes,
                                                    int64_t rx_bytes) {
  return std::unique_ptr<DataUse>(
      new DataUse(url, base::TimeTicks() /* request_start */,
                  GURL("http://examplefirstparty.com"), tab_id,
                  net::NetworkChangeNotifier::CONNECTION_2G, "example_mcc_mnc",
                  tx_bytes, rx_bytes));
}

// Synthesizes a fake std::unique_ptr<DataUse> with the given |url|, |tx_bytes|
// and
// |rx_bytes|, using arbitrary values for all other fields.
std::unique_ptr<DataUse> CreateDataUseWithURL(const GURL& url,
                                              int64_t tx_bytes,
                                              int64_t rx_bytes) {
  return CreateDataUseWithURLAndTab(url, SessionID::FromSerializedValue(10),
                                    tx_bytes, rx_bytes);
}

// Synthesizes a fake std::unique_ptr<DataUse> with the given |tab_id|,
// |tx_bytes|
// and |rx_bytes|, using arbitrary values for all other fields.
std::unique_ptr<DataUse> CreateDataUseWithTab(SessionID tab_id,
                                              int64_t tx_bytes,
                                              int64_t rx_bytes) {
  return CreateDataUseWithURLAndTab(GURL("http://example.com"), tab_id,
                                    tx_bytes, rx_bytes);
}

// Synthesizes a fake std::unique_ptr<DataUse> with the given |tx_bytes| and
// |rx_bytes|, using arbitrary values for all other fields.
std::unique_ptr<DataUse> CreateDataUse(int64_t tx_bytes, int64_t rx_bytes) {
  return CreateDataUseWithURL(GURL("http://example.com"), tx_bytes, rx_bytes);
}

// Appends |data_use| to |data_use_sequence|. |data_use_sequence| must not be
// NULL.
void AppendDataUseToSequence(
    std::vector<std::unique_ptr<DataUse>>* data_use_sequence,
    std::unique_ptr<DataUse> data_use) {
  data_use_sequence->push_back(std::move(data_use));
}

// Class that represents a base::MockTimer with an attached base::TickClock, so
// that it can update its |desired_run_time()| according to the current time
// when the timer is reset.
class MockTimerWithTickClock : public base::MockTimer {
 public:
  MockTimerWithTickClock(bool retain_user_task,
                         bool is_repeating,
                         const base::TickClock* tick_clock)
      : base::MockTimer(retain_user_task, is_repeating),
        tick_clock_(tick_clock) {}

  ~MockTimerWithTickClock() override {}

  void Reset() override {
    base::MockTimer::Reset();
    set_desired_run_time(tick_clock_->NowTicks() + GetCurrentDelay());
  }

 private:
  const base::TickClock* tick_clock_;

  DISALLOW_COPY_AND_ASSIGN(MockTimerWithTickClock);
};

// A TrafficStatsAmortizer for testing that allows for tests to simulate the
// byte counts returned from TrafficStats.
class TestTrafficStatsAmortizer : public TrafficStatsAmortizer {
 public:
  TestTrafficStatsAmortizer(
      const base::TickClock* tick_clock,
      std::unique_ptr<base::Timer> traffic_stats_query_timer)
      : TrafficStatsAmortizer(tick_clock,
                              std::move(traffic_stats_query_timer),
                              kTrafficStatsQueryDelay,
                              kMaxAmortizationDelay,
                              kMaxDataUseBufferSize),
        next_traffic_stats_available_(false),
        next_traffic_stats_tx_bytes_(-1),
        next_traffic_stats_rx_bytes_(-1) {}

  ~TestTrafficStatsAmortizer() override {}

  void SetNextTrafficStats(bool available, int64_t tx_bytes, int64_t rx_bytes) {
    next_traffic_stats_available_ = available;
    next_traffic_stats_tx_bytes_ = tx_bytes;
    next_traffic_stats_rx_bytes_ = rx_bytes;
  }

  void AddTrafficStats(int64_t tx_bytes, int64_t rx_bytes) {
    next_traffic_stats_tx_bytes_ += tx_bytes;
    next_traffic_stats_rx_bytes_ += rx_bytes;
  }

 protected:
  bool QueryTrafficStats(int64_t* tx_bytes, int64_t* rx_bytes) const override {
    *tx_bytes = next_traffic_stats_tx_bytes_;
    *rx_bytes = next_traffic_stats_rx_bytes_;
    return next_traffic_stats_available_;
  }

 private:
  bool next_traffic_stats_available_;
  int64_t next_traffic_stats_tx_bytes_;
  int64_t next_traffic_stats_rx_bytes_;

  DISALLOW_COPY_AND_ASSIGN(TestTrafficStatsAmortizer);
};

class TrafficStatsAmortizerTest : public testing::Test {
 public:
  TrafficStatsAmortizerTest()
      : mock_timer_(
            new MockTimerWithTickClock(false, false, &test_tick_clock_)),
        amortizer_(&test_tick_clock_,
                   std::unique_ptr<base::Timer>(mock_timer_)),
        data_use_callback_call_count_(0) {}

  ~TrafficStatsAmortizerTest() override {
    EXPECT_FALSE(mock_timer_->IsRunning());
  }

  // Simulates the passage of time by |delta|, firing timers when appropriate.
  void AdvanceTime(const base::TimeDelta& delta) {
    const base::TimeTicks end_time = test_tick_clock_.NowTicks() + delta;
    base::RunLoop().RunUntilIdle();

    while (test_tick_clock_.NowTicks() < end_time) {
      PumpMockTimer();

      // If |mock_timer_| is scheduled to fire in the future before |end_time|,
      // advance to that time.
      if (mock_timer_->IsRunning() &&
          mock_timer_->desired_run_time() < end_time) {
        test_tick_clock_.Advance(mock_timer_->desired_run_time() -
                                 test_tick_clock_.NowTicks());
      } else {
        // Otherwise, advance to |end_time|.
        test_tick_clock_.Advance(end_time - test_tick_clock_.NowTicks());
      }
    }
    PumpMockTimer();
  }

  // Skip the first amortization run where TrafficStats byte count deltas are
  // unavailable, for convenience.
  void SkipFirstAmortizationRun() {
    // The initial values of TrafficStats shouldn't matter.
    amortizer()->SetNextTrafficStats(true, 0, 0);

    // Do the first amortization run with TrafficStats unavailable.
    amortizer()->OnExtraBytes(100, 1000);
    AdvanceTime(kTrafficStatsQueryDelay);
    EXPECT_EQ(0, data_use_callback_call_count());
  }

  // Expects that |expected| and |actual| are equivalent.
  void ExpectDataUse(std::unique_ptr<DataUse> expected,
                     std::unique_ptr<DataUse> actual) {
    ++data_use_callback_call_count_;

    // Have separate checks for the |tx_bytes| and |rx_bytes|, since those are
    // calculated with floating point arithmetic.
    EXPECT_DOUBLE_EQ(static_cast<double>(expected->tx_bytes),
                     static_cast<double>(actual->tx_bytes));
    EXPECT_DOUBLE_EQ(static_cast<double>(expected->rx_bytes),
                     static_cast<double>(actual->rx_bytes));

    // Copy the byte counts over from |expected| just in case they're only
    // slightly different due to floating point error, so that this doesn't
    // cause the equality comparison below to fail.
    actual->tx_bytes = expected->tx_bytes;
    actual->rx_bytes = expected->rx_bytes;
    EXPECT_EQ(*expected, *actual);
  }

  // Creates an ExpectDataUse callback, as a convenience.
  DataUseAmortizer::AmortizationCompleteCallback ExpectDataUseCallback(
      std::unique_ptr<DataUse> expected) {
    return base::Bind(&TrafficStatsAmortizerTest::ExpectDataUse,
                      base::Unretained(this), base::Passed(&expected));
  }

  base::TimeTicks NowTicks() { return test_tick_clock_.NowTicks(); }

  TestTrafficStatsAmortizer* amortizer() { return &amortizer_; }

  int data_use_callback_call_count() const {
    return data_use_callback_call_count_;
  }

 private:
  // Pumps |mock_timer_|, firing it while it's scheduled to run now or in the
  // past. After calling this, |mock_timer_| is either not running or is
  // scheduled to run in the future.
  void PumpMockTimer() {
    // Fire the |mock_timer_| if the time has come up. Use a while loop in case
    // the fired task started the timer again to fire immediately.
    while (mock_timer_->IsRunning() &&
           mock_timer_->desired_run_time() <= test_tick_clock_.NowTicks()) {
      mock_timer_->Fire();
      base::RunLoop().RunUntilIdle();
    }
  }

  base::MessageLoop message_loop_;

  base::SimpleTestTickClock test_tick_clock_;

  // Weak, owned by |amortizer_|.
  MockTimerWithTickClock* mock_timer_;

  TestTrafficStatsAmortizer amortizer_;

  // The number of times ExpectDataUse has been called.
  int data_use_callback_call_count_;

  DISALLOW_COPY_AND_ASSIGN(TrafficStatsAmortizerTest);
};

TEST_F(TrafficStatsAmortizerTest, AmortizeWithTrafficStatsAlwaysUnavailable) {
  amortizer()->SetNextTrafficStats(false, -1, -1);
  // Do it three times for good measure.
  for (int i = 0; i < 3; ++i) {
    base::HistogramTester histogram_tester;

    // Extra bytes should be ignored since TrafficStats are unavailable.
    amortizer()->OnExtraBytes(1337, 9001);
    // The original DataUse should be unchanged.
    amortizer()->AmortizeDataUse(
        CreateDataUse(100, 1000),
        ExpectDataUseCallback(CreateDataUse(100, 1000)));

    AdvanceTime(kTrafficStatsQueryDelay);
    EXPECT_EQ(i + 1, data_use_callback_call_count());

    histogram_tester.ExpectUniqueSample(kPreAmortizationTxHistogram, 100, 1);
    histogram_tester.ExpectUniqueSample(kPreAmortizationRxHistogram, 1000, 1);
    histogram_tester.ExpectUniqueSample(kPostAmortizationTxHistogram, 100, 1);
    histogram_tester.ExpectUniqueSample(kPostAmortizationRxHistogram, 1000, 1);
    histogram_tester.ExpectUniqueSample(kAmortizationDelayHistogram,
                                        GetDelaySample(kTrafficStatsQueryDelay),
                                        1);
    histogram_tester.ExpectUniqueSample(kBufferSizeOnFlushHistogram, 1, 1);
    histogram_tester.ExpectUniqueSample(kConcurrentTabs, 1, 1);
  }
}

TEST_F(TrafficStatsAmortizerTest, AmortizeDataUse) {
  // Simulate the first amortization run.
  {
    base::HistogramTester histogram_tester;

    // The initial values of TrafficStats shouldn't matter.
    amortizer()->SetNextTrafficStats(true, 1337, 9001);

    // The first amortization run should not change any byte counts because
    // there's no TrafficStats delta to work with.
    amortizer()->AmortizeDataUse(CreateDataUse(50, 500),
                                 ExpectDataUseCallback(CreateDataUse(50, 500)));
    amortizer()->AmortizeDataUse(
        CreateDataUse(100, 1000),
        ExpectDataUseCallback(CreateDataUse(100, 1000)));
    AdvanceTime(kTrafficStatsQueryDelay);
    EXPECT_EQ(2, data_use_callback_call_count());

    histogram_tester.ExpectUniqueSample(kPreAmortizationTxHistogram, 150, 1);
    histogram_tester.ExpectUniqueSample(kPreAmortizationRxHistogram, 1500, 1);
    histogram_tester.ExpectUniqueSample(kPostAmortizationTxHistogram, 150, 1);
    histogram_tester.ExpectUniqueSample(kPostAmortizationRxHistogram, 1500, 1);
    histogram_tester.ExpectUniqueSample(kAmortizationDelayHistogram,
                                        GetDelaySample(kTrafficStatsQueryDelay),
                                        1);
    histogram_tester.ExpectUniqueSample(kBufferSizeOnFlushHistogram, 2, 1);
    histogram_tester.ExpectUniqueSample(kConcurrentTabs, 1, 1);
  }

  // Simulate the second amortization run.
  {
    base::HistogramTester histogram_tester;

    // This amortization run, tx_bytes and rx_bytes should be doubled.
    amortizer()->AmortizeDataUse(
        CreateDataUse(50, 500),
        ExpectDataUseCallback(CreateDataUse(100, 1000)));
    AdvanceTime(kTrafficStatsQueryDelay / 2);

    // Another DataUse is reported before the amortizer queries TrafficStats.
    amortizer()->AmortizeDataUse(
        CreateDataUse(100, 1000),
        ExpectDataUseCallback(CreateDataUse(200, 2000)));
    AdvanceTime(kTrafficStatsQueryDelay / 2);

    // Then, the TrafficStats values update with the new bytes. The second run
    // callbacks should not have been called yet.
    amortizer()->AddTrafficStats(300, 3000);
    EXPECT_EQ(2, data_use_callback_call_count());

    // The callbacks should fire once kTrafficStatsQueryDelay has passed since
    // the DataUse was passed to the amortizer.
    AdvanceTime(kTrafficStatsQueryDelay / 2);
    EXPECT_EQ(4, data_use_callback_call_count());

    histogram_tester.ExpectUniqueSample(kPreAmortizationTxHistogram, 150, 1);
    histogram_tester.ExpectUniqueSample(kPreAmortizationRxHistogram, 1500, 1);
    histogram_tester.ExpectUniqueSample(kPostAmortizationTxHistogram, 300, 1);
    histogram_tester.ExpectUniqueSample(kPostAmortizationRxHistogram, 3000, 1);
    histogram_tester.ExpectUniqueSample(
        kAmortizationDelayHistogram,
        GetDelaySample(kTrafficStatsQueryDelay + kTrafficStatsQueryDelay / 2),
        1);
    histogram_tester.ExpectUniqueSample(kBufferSizeOnFlushHistogram, 2, 1);
    histogram_tester.ExpectUniqueSample(kConcurrentTabs, 1, 1);
  }
}

TEST_F(TrafficStatsAmortizerTest, AmortizeWithExtraBytes) {
  SkipFirstAmortizationRun();
  base::HistogramTester histogram_tester;

  // Byte counts should double.
  amortizer()->AmortizeDataUse(CreateDataUse(50, 500),
                               ExpectDataUseCallback(CreateDataUse(100, 1000)));
  amortizer()->OnExtraBytes(500, 5000);
  amortizer()->AddTrafficStats(1100, 11000);
  AdvanceTime(kTrafficStatsQueryDelay);
  EXPECT_EQ(1, data_use_callback_call_count());

  histogram_tester.ExpectUniqueSample(kPreAmortizationTxHistogram, 50, 1);
  histogram_tester.ExpectUniqueSample(kPreAmortizationRxHistogram, 500, 1);
  histogram_tester.ExpectUniqueSample(kPostAmortizationTxHistogram, 100, 1);
  histogram_tester.ExpectUniqueSample(kPostAmortizationRxHistogram, 1000, 1);
  histogram_tester.ExpectUniqueSample(
      kAmortizationDelayHistogram, GetDelaySample(kTrafficStatsQueryDelay), 1);
  histogram_tester.ExpectUniqueSample(kBufferSizeOnFlushHistogram, 1, 1);
  histogram_tester.ExpectUniqueSample(kConcurrentTabs, 1, 1);
}

TEST_F(TrafficStatsAmortizerTest, AmortizeWithNegativeOverhead) {
  SkipFirstAmortizationRun();
  base::HistogramTester histogram_tester;

  // Byte counts should halve.
  amortizer()->AmortizeDataUse(CreateDataUse(50, 500),
                               ExpectDataUseCallback(CreateDataUse(25, 250)));
  amortizer()->AddTrafficStats(25, 250);
  AdvanceTime(kTrafficStatsQueryDelay);
  EXPECT_EQ(1, data_use_callback_call_count());

  histogram_tester.ExpectUniqueSample(kPreAmortizationTxHistogram, 50, 1);
  histogram_tester.ExpectUniqueSample(kPreAmortizationRxHistogram, 500, 1);
  histogram_tester.ExpectUniqueSample(kPostAmortizationTxHistogram, 25, 1);
  histogram_tester.ExpectUniqueSample(kPostAmortizationRxHistogram, 250, 1);
  histogram_tester.ExpectUniqueSample(
      kAmortizationDelayHistogram, GetDelaySample(kTrafficStatsQueryDelay), 1);
  histogram_tester.ExpectUniqueSample(kBufferSizeOnFlushHistogram, 1, 1);
  histogram_tester.ExpectUniqueSample(kConcurrentTabs, 1, 1);
}

TEST_F(TrafficStatsAmortizerTest, AmortizeWithMaxIntByteCounts) {
  SkipFirstAmortizationRun();
  base::HistogramTester histogram_tester;

  // Byte counts should be unchanged.
  amortizer()->AmortizeDataUse(
      CreateDataUse(INT64_MAX, INT64_MAX),
      ExpectDataUseCallback(CreateDataUse(INT64_MAX, INT64_MAX)));
  amortizer()->SetNextTrafficStats(true, INT64_MAX, INT64_MAX);
  AdvanceTime(kTrafficStatsQueryDelay);
  EXPECT_EQ(1, data_use_callback_call_count());

  // Byte count samples should be capped at the maximum Sample value that's
  // valid to be recorded.
  histogram_tester.ExpectUniqueSample(kPreAmortizationTxHistogram,
                                      kMaxRecordableSample, 1);
  histogram_tester.ExpectUniqueSample(kPreAmortizationRxHistogram,
                                      kMaxRecordableSample, 1);
  histogram_tester.ExpectUniqueSample(kPostAmortizationTxHistogram,
                                      kMaxRecordableSample, 1);
  histogram_tester.ExpectUniqueSample(kPostAmortizationRxHistogram,
                                      kMaxRecordableSample, 1);
  histogram_tester.ExpectUniqueSample(
      kAmortizationDelayHistogram, GetDelaySample(kTrafficStatsQueryDelay), 1);
  histogram_tester.ExpectUniqueSample(kBufferSizeOnFlushHistogram, 1, 1);
  histogram_tester.ExpectUniqueSample(kConcurrentTabs, 1, 1);
}

TEST_F(TrafficStatsAmortizerTest, AmortizeWithMaxIntScaleFactor) {
  SkipFirstAmortizationRun();
  base::HistogramTester histogram_tester;

  // Byte counts should be scaled up to INT64_MAX.
  amortizer()->AmortizeDataUse(
      CreateDataUse(1, 1),
      ExpectDataUseCallback(CreateDataUse(INT64_MAX, INT64_MAX)));
  amortizer()->SetNextTrafficStats(true, INT64_MAX, INT64_MAX);
  AdvanceTime(kTrafficStatsQueryDelay);
  EXPECT_EQ(1, data_use_callback_call_count());

  // Post-amortization byte count samples should be capped at the maximum Sample
  // value that's valid to be recorded.
  histogram_tester.ExpectUniqueSample(kPreAmortizationTxHistogram, 1, 1);
  histogram_tester.ExpectUniqueSample(kPreAmortizationRxHistogram, 1, 1);
  histogram_tester.ExpectUniqueSample(kPostAmortizationTxHistogram,
                                      kMaxRecordableSample, 1);
  histogram_tester.ExpectUniqueSample(kPostAmortizationRxHistogram,
                                      kMaxRecordableSample, 1);
  histogram_tester.ExpectUniqueSample(
      kAmortizationDelayHistogram, GetDelaySample(kTrafficStatsQueryDelay), 1);
  histogram_tester.ExpectUniqueSample(kBufferSizeOnFlushHistogram, 1, 1);
  histogram_tester.ExpectUniqueSample(kConcurrentTabs, 1, 1);
}

TEST_F(TrafficStatsAmortizerTest, AmortizeWithZeroScaleFactor) {
  SkipFirstAmortizationRun();
  base::HistogramTester histogram_tester;

  // Byte counts should be scaled down to 0.
  amortizer()->AmortizeDataUse(CreateDataUse(INT64_MAX, INT64_MAX),
                               ExpectDataUseCallback(CreateDataUse(0, 0)));
  amortizer()->SetNextTrafficStats(true, 0, 0);
  AdvanceTime(kTrafficStatsQueryDelay);
  EXPECT_EQ(1, data_use_callback_call_count());

  // Pre-amortization byte count samples should be capped at the maximum Sample
  // value that's valid to be recorded.
  histogram_tester.ExpectUniqueSample(kPreAmortizationTxHistogram,
                                      kMaxRecordableSample, 1);
  histogram_tester.ExpectUniqueSample(kPreAmortizationRxHistogram,
                                      kMaxRecordableSample, 1);
  histogram_tester.ExpectUniqueSample(kPostAmortizationTxHistogram, 0, 1);
  histogram_tester.ExpectUniqueSample(kPostAmortizationRxHistogram, 0, 1);
  histogram_tester.ExpectUniqueSample(
      kAmortizationDelayHistogram, GetDelaySample(kTrafficStatsQueryDelay), 1);
  histogram_tester.ExpectUniqueSample(kBufferSizeOnFlushHistogram, 1, 1);
  histogram_tester.ExpectUniqueSample(kConcurrentTabs, 1, 1);
}

TEST_F(TrafficStatsAmortizerTest, AmortizeWithZeroPreAmortizationBytes) {
  SkipFirstAmortizationRun();

  {
    base::HistogramTester histogram_tester;

    // Both byte counts should stay 0, even though TrafficStats saw bytes, which
    // should be reflected in the next amortization run instead.
    amortizer()->AmortizeDataUse(CreateDataUse(0, 0),
                                 ExpectDataUseCallback(CreateDataUse(0, 0)));
    // Add the TrafficStats byte counts for this and the next amortization run.
    amortizer()->AddTrafficStats(100, 1000);
    AdvanceTime(kTrafficStatsQueryDelay);
    EXPECT_EQ(1, data_use_callback_call_count());

    histogram_tester.ExpectUniqueSample(kPreAmortizationTxHistogram, 0, 1);
    histogram_tester.ExpectUniqueSample(kPreAmortizationRxHistogram, 0, 1);
    histogram_tester.ExpectUniqueSample(kPostAmortizationTxHistogram, 0, 1);
    histogram_tester.ExpectUniqueSample(kPostAmortizationRxHistogram, 0, 1);
    histogram_tester.ExpectUniqueSample(kAmortizationDelayHistogram,
                                        GetDelaySample(kTrafficStatsQueryDelay),
                                        1);
    histogram_tester.ExpectUniqueSample(kBufferSizeOnFlushHistogram, 1, 1);
    histogram_tester.ExpectUniqueSample(kConcurrentTabs, 1, 1);
  }

  {
    base::HistogramTester histogram_tester;

    // Both byte counts should double, even though the TrafficStats byte counts
    // actually updated during the previous amortization run.
    amortizer()->AmortizeDataUse(
        CreateDataUse(50, 500),
        ExpectDataUseCallback(CreateDataUse(100, 1000)));
    AdvanceTime(kTrafficStatsQueryDelay);
    EXPECT_EQ(2, data_use_callback_call_count());

    histogram_tester.ExpectUniqueSample(kPreAmortizationTxHistogram, 50, 1);
    histogram_tester.ExpectUniqueSample(kPreAmortizationRxHistogram, 500, 1);
    histogram_tester.ExpectUniqueSample(kPostAmortizationTxHistogram, 100, 1);
    histogram_tester.ExpectUniqueSample(kPostAmortizationRxHistogram, 1000, 1);
    histogram_tester.ExpectUniqueSample(kAmortizationDelayHistogram,
                                        GetDelaySample(kTrafficStatsQueryDelay),
                                        1);
    histogram_tester.ExpectUniqueSample(kBufferSizeOnFlushHistogram, 1, 1);
    histogram_tester.ExpectUniqueSample(kConcurrentTabs, 1, 1);
  }
}

TEST_F(TrafficStatsAmortizerTest, AmortizeWithZeroTxPreAmortizationBytes) {
  SkipFirstAmortizationRun();

  {
    base::HistogramTester histogram_tester;

    // The count of transmitted bytes starts at 0, so it should stay 0, and be
    // amortized in the next amortization run instead.
    amortizer()->AmortizeDataUse(CreateDataUse(0, 500),
                                 ExpectDataUseCallback(CreateDataUse(0, 1000)));
    // Add the TrafficStats byte counts for this and the next amortization run.
    amortizer()->AddTrafficStats(100, 1000);
    AdvanceTime(kTrafficStatsQueryDelay);
    EXPECT_EQ(1, data_use_callback_call_count());

    histogram_tester.ExpectUniqueSample(kPreAmortizationTxHistogram, 0, 1);
    histogram_tester.ExpectUniqueSample(kPreAmortizationRxHistogram, 500, 1);
    histogram_tester.ExpectUniqueSample(kPostAmortizationTxHistogram, 0, 1);
    histogram_tester.ExpectUniqueSample(kPostAmortizationRxHistogram, 1000, 1);
    histogram_tester.ExpectUniqueSample(kAmortizationDelayHistogram,
                                        GetDelaySample(kTrafficStatsQueryDelay),
                                        1);
    histogram_tester.ExpectUniqueSample(kBufferSizeOnFlushHistogram, 1, 1);
    histogram_tester.ExpectUniqueSample(kConcurrentTabs, 1, 1);
  }

  {
    base::HistogramTester histogram_tester;

    // The count of transmitted bytes should double, even though they actually
    // updated during the previous amortization run.
    amortizer()->AmortizeDataUse(CreateDataUse(50, 0),
                                 ExpectDataUseCallback(CreateDataUse(100, 0)));
    AdvanceTime(kTrafficStatsQueryDelay);
    EXPECT_EQ(2, data_use_callback_call_count());

    histogram_tester.ExpectUniqueSample(kPreAmortizationTxHistogram, 50, 1);
    histogram_tester.ExpectUniqueSample(kPreAmortizationRxHistogram, 0, 1);
    histogram_tester.ExpectUniqueSample(kPostAmortizationTxHistogram, 100, 1);
    histogram_tester.ExpectUniqueSample(kPostAmortizationRxHistogram, 0, 1);
    histogram_tester.ExpectUniqueSample(kAmortizationDelayHistogram,
                                        GetDelaySample(kTrafficStatsQueryDelay),
                                        1);
    histogram_tester.ExpectUniqueSample(kBufferSizeOnFlushHistogram, 1, 1);
    histogram_tester.ExpectUniqueSample(kConcurrentTabs, 1, 1);
  }
}

TEST_F(TrafficStatsAmortizerTest, AmortizeWithZeroRxPreAmortizationBytes) {
  SkipFirstAmortizationRun();

  {
    base::HistogramTester histogram_tester;

    // The count of received bytes starts at 0, so it should stay 0, and be
    // amortized in the next amortization run instead.
    amortizer()->AmortizeDataUse(CreateDataUse(50, 0),
                                 ExpectDataUseCallback(CreateDataUse(100, 0)));
    // Add the TrafficStats byte counts for this and the next amortization run.
    amortizer()->AddTrafficStats(100, 1000);
    AdvanceTime(kTrafficStatsQueryDelay);
    EXPECT_EQ(1, data_use_callback_call_count());

    histogram_tester.ExpectUniqueSample(kPreAmortizationTxHistogram, 50, 1);
    histogram_tester.ExpectUniqueSample(kPreAmortizationRxHistogram, 0, 1);
    histogram_tester.ExpectUniqueSample(kPostAmortizationTxHistogram, 100, 1);
    histogram_tester.ExpectUniqueSample(kPostAmortizationRxHistogram, 0, 1);
    histogram_tester.ExpectUniqueSample(kAmortizationDelayHistogram,
                                        GetDelaySample(kTrafficStatsQueryDelay),
                                        1);
    histogram_tester.ExpectUniqueSample(kBufferSizeOnFlushHistogram, 1, 1);
    histogram_tester.ExpectUniqueSample(kConcurrentTabs, 1, 1);
  }

  {
    base::HistogramTester histogram_tester;

    // The count of received bytes should double, even though they actually
    // updated during the previous amortization run.
    amortizer()->AmortizeDataUse(CreateDataUse(0, 500),
                                 ExpectDataUseCallback(CreateDataUse(0, 1000)));
    AdvanceTime(kTrafficStatsQueryDelay);
    EXPECT_EQ(2, data_use_callback_call_count());

    histogram_tester.ExpectUniqueSample(kPreAmortizationTxHistogram, 0, 1);
    histogram_tester.ExpectUniqueSample(kPreAmortizationRxHistogram, 500, 1);
    histogram_tester.ExpectUniqueSample(kPostAmortizationTxHistogram, 0, 1);
    histogram_tester.ExpectUniqueSample(kPostAmortizationRxHistogram, 1000, 1);
    histogram_tester.ExpectUniqueSample(kAmortizationDelayHistogram,
                                        GetDelaySample(kTrafficStatsQueryDelay),
                                        1);
    histogram_tester.ExpectUniqueSample(kBufferSizeOnFlushHistogram, 1, 1);
    histogram_tester.ExpectUniqueSample(kConcurrentTabs, 1, 1);
  }
}

TEST_F(TrafficStatsAmortizerTest, AmortizeAtMaxDelay) {
  SkipFirstAmortizationRun();
  base::HistogramTester histogram_tester;

  // Byte counts should double.
  amortizer()->AddTrafficStats(1000, 10000);
  amortizer()->AmortizeDataUse(CreateDataUse(50, 500),
                               ExpectDataUseCallback(CreateDataUse(100, 1000)));

  // kSmallDelay is a delay that's shorter than the delay before TrafficStats
  // would be queried, where kMaxAmortizationDelay is a multiple of kSmallDelay.
  const base::TimeDelta kSmallDelay = kMaxAmortizationDelay / 10;
  EXPECT_LT(kSmallDelay, kTrafficStatsQueryDelay);

  // Simulate multiple cases of extra bytes being reported, each before
  // TrafficStats would be queried, until kMaxAmortizationDelay has elapsed.
  AdvanceTime(kSmallDelay);
  for (int64_t i = 0; i < kMaxAmortizationDelay / kSmallDelay - 1; ++i) {
    EXPECT_EQ(0, data_use_callback_call_count());
    amortizer()->OnExtraBytes(50, 500);
    AdvanceTime(kSmallDelay);
  }

  // The final time, the amortizer should have given up on waiting to query
  // TrafficStats and just have amortized as soon as it hit the deadline of
  // kMaxAmortizationDelay.
  EXPECT_EQ(1, data_use_callback_call_count());

  histogram_tester.ExpectUniqueSample(kPreAmortizationTxHistogram, 50, 1);
  histogram_tester.ExpectUniqueSample(kPreAmortizationRxHistogram, 500, 1);
  histogram_tester.ExpectUniqueSample(kPostAmortizationTxHistogram, 100, 1);
  histogram_tester.ExpectUniqueSample(kPostAmortizationRxHistogram, 1000, 1);
  histogram_tester.ExpectUniqueSample(kAmortizationDelayHistogram,
                                      GetDelaySample(kMaxAmortizationDelay), 1);
  histogram_tester.ExpectUniqueSample(kBufferSizeOnFlushHistogram, 1, 1);
  histogram_tester.ExpectUniqueSample(kConcurrentTabs, 1, 1);
}

TEST_F(TrafficStatsAmortizerTest, AmortizeAtMaxBufferSize) {
  SkipFirstAmortizationRun();
  base::HistogramTester histogram_tester;

  // Report (max buffer size + 1) consecutive DataUse objects, which will be
  // amortized immediately once the buffer exceeds maximum size.
  amortizer()->AddTrafficStats(100 * (kMaxDataUseBufferSize + 1),
                               1000 * (kMaxDataUseBufferSize + 1));
  for (size_t i = 0; i < kMaxDataUseBufferSize + 1; ++i) {
    EXPECT_EQ(0, data_use_callback_call_count());
    amortizer()->AmortizeDataUse(
        CreateDataUse(50, 500),
        ExpectDataUseCallback(CreateDataUse(100, 1000)));
  }

  const int kExpectedBufSize = static_cast<int>(kMaxDataUseBufferSize + 1);
  EXPECT_EQ(kExpectedBufSize, data_use_callback_call_count());

  histogram_tester.ExpectUniqueSample(kPreAmortizationTxHistogram,
                                      50 * kExpectedBufSize, 1);
  histogram_tester.ExpectUniqueSample(kPreAmortizationRxHistogram,
                                      500 * kExpectedBufSize, 1);
  histogram_tester.ExpectUniqueSample(kPostAmortizationTxHistogram,
                                      100 * kExpectedBufSize, 1);
  histogram_tester.ExpectUniqueSample(kPostAmortizationRxHistogram,
                                      1000 * kExpectedBufSize, 1);
  histogram_tester.ExpectUniqueSample(kAmortizationDelayHistogram, 0, 1);
  histogram_tester.ExpectUniqueSample(kBufferSizeOnFlushHistogram,
                                      kExpectedBufSize, 1);
  histogram_tester.ExpectUniqueSample(kConcurrentTabs, 1, 1);
}

TEST_F(TrafficStatsAmortizerTest, AmortizeCombinedDataUse) {
  SkipFirstAmortizationRun();
  base::HistogramTester histogram_tester;

  const GURL foo_url("http://foo.com");
  const GURL bar_url("http://bar.com");

  std::vector<std::unique_ptr<DataUse>> baz_sequence;
  const DataUseAmortizer::AmortizationCompleteCallback baz_callback =
      base::Bind(&AppendDataUseToSequence, &baz_sequence);

  std::vector<std::unique_ptr<DataUse>> qux_sequence;
  const DataUseAmortizer::AmortizationCompleteCallback qux_callback =
      base::Bind(&AppendDataUseToSequence, &qux_sequence);

  // Byte counts should double, with some DataUse objects combined together.

  // Two consecutive DataUse objects that are identical except for byte counts
  // and with the same callback should be combined.
  amortizer()->AmortizeDataUse(CreateDataUseWithURL(foo_url, 50, 500),
                               baz_callback);
  amortizer()->AmortizeDataUse(CreateDataUseWithURL(foo_url, 100, 1000),
                               baz_callback);

  // This DataUse object should not be combined with the previous one because it
  // has a different URL.
  amortizer()->AmortizeDataUse(CreateDataUseWithURL(bar_url, 50, 500),
                               baz_callback);

  // This DataUse object should not be combined with the previous one because it
  // has a different callback.
  amortizer()->AmortizeDataUse(CreateDataUseWithURL(bar_url, 50, 500),
                               qux_callback);

  // This DataUse object should not be combined with the previous foo/baz
  // DataUse objects because other DataUse objects were reported in-between.
  amortizer()->AmortizeDataUse(CreateDataUseWithURL(foo_url, 50, 500),
                               baz_callback);

  // Simulate that TrafficStats saw double the number of reported bytes across
  // all reported DataUse.
  amortizer()->AddTrafficStats(600, 6000);
  AdvanceTime(kTrafficStatsQueryDelay);

  EXPECT_EQ(3U, baz_sequence.size());
  ExpectDataUse(CreateDataUseWithURL(foo_url, 300, 3000),
                std::move(baz_sequence[0]));
  ExpectDataUse(CreateDataUseWithURL(bar_url, 100, 1000),
                std::move(baz_sequence[1]));
  ExpectDataUse(CreateDataUseWithURL(foo_url, 100, 1000),
                std::move(baz_sequence[2]));

  EXPECT_EQ(1U, qux_sequence.size());
  ExpectDataUse(CreateDataUseWithURL(bar_url, 100, 1000),
                std::move(qux_sequence[0]));

  histogram_tester.ExpectUniqueSample(kPreAmortizationTxHistogram, 300, 1);
  histogram_tester.ExpectUniqueSample(kPreAmortizationRxHistogram, 3000, 1);
  histogram_tester.ExpectUniqueSample(kPostAmortizationTxHistogram, 600, 1);
  histogram_tester.ExpectUniqueSample(kPostAmortizationRxHistogram, 6000, 1);
  histogram_tester.ExpectUniqueSample(
      kAmortizationDelayHistogram, GetDelaySample(kTrafficStatsQueryDelay), 1);
  histogram_tester.ExpectUniqueSample(kBufferSizeOnFlushHistogram, 4, 1);
  histogram_tester.ExpectUniqueSample(kConcurrentTabs, 1, 1);
}

TEST_F(TrafficStatsAmortizerTest, ConcurrentTabsHistogram) {
  SessionID kTabId1 = SessionID::FromSerializedValue(1);
  SessionID kTabId2 = SessionID::FromSerializedValue(2);

  SkipFirstAmortizationRun();

  {
    // Test data usage reported multiple times for two tabs.
    base::HistogramTester histogram_tester;
    amortizer()->SetNextTrafficStats(true, 0, 0);
    amortizer()->AmortizeDataUse(
        CreateDataUseWithTab(kTabId1, 50, 500),
        ExpectDataUseCallback(CreateDataUseWithTab(kTabId1, 100, 1000)));
    amortizer()->AmortizeDataUse(
        CreateDataUseWithTab(kTabId2, 100, 1000),
        ExpectDataUseCallback(CreateDataUseWithTab(kTabId2, 200, 2000)));
    amortizer()->AmortizeDataUse(
        CreateDataUseWithTab(kTabId1, 50, 500),
        ExpectDataUseCallback(CreateDataUseWithTab(kTabId1, 100, 1000)));
    amortizer()->AmortizeDataUse(
        CreateDataUseWithTab(kTabId2, 100, 1000),
        ExpectDataUseCallback(CreateDataUseWithTab(kTabId2, 200, 2000)));
    amortizer()->SetNextTrafficStats(true, 600, 6000);
    AdvanceTime(kTrafficStatsQueryDelay);
    histogram_tester.ExpectUniqueSample(kConcurrentTabs, 2, 1);
    histogram_tester.ExpectUniqueSample(kPreAmortizationTxHistogram, 300, 1);
    histogram_tester.ExpectUniqueSample(kPreAmortizationRxHistogram, 3000, 1);
    histogram_tester.ExpectUniqueSample(kPostAmortizationTxHistogram, 600, 1);
    histogram_tester.ExpectUniqueSample(kPostAmortizationRxHistogram, 6000, 1);
  }

  // Test data usage for 1-5 tabs.
  for (int32_t total_tabs = 1; total_tabs <= 5; ++total_tabs) {
    base::HistogramTester histogram_tester;

    for (int32_t i = 1; i <= total_tabs; ++i) {
      SessionID tab_id = SessionID::FromSerializedValue(i);
      amortizer()->AmortizeDataUse(
          CreateDataUseWithTab(tab_id, 100, 1000),
          ExpectDataUseCallback(CreateDataUseWithTab(tab_id, 200, 2000)));
    }
    amortizer()->AddTrafficStats(total_tabs * 200, total_tabs * 2000);
    AdvanceTime(kTrafficStatsQueryDelay);
    histogram_tester.ExpectUniqueSample(kConcurrentTabs, total_tabs, 1);
  }
}

}  // namespace

}  // namespace android
}  // namespace data_usage
