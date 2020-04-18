// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/page_load_metrics/metrics_render_frame_observer.h"

#include <memory>
#include <utility>

#include "base/memory/ptr_util.h"
#include "base/time/time.h"
#include "base/timer/mock_timer.h"
#include "chrome/common/page_load_metrics/page_load_timing.h"
#include "chrome/common/page_load_metrics/test/weak_mock_timer.h"
#include "chrome/renderer/page_load_metrics/fake_page_timing_sender.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace page_load_metrics {

// Implementation of the MetricsRenderFrameObserver class we're testing, with
// the GetTiming() method stubbed out to make the rest of the class more
// testable.
class TestMetricsRenderFrameObserver : public MetricsRenderFrameObserver,
                                       public test::WeakMockTimerProvider {
 public:
  TestMetricsRenderFrameObserver() : MetricsRenderFrameObserver(nullptr) {}

  std::unique_ptr<base::Timer> CreateTimer() override {
    auto timer = std::make_unique<test::WeakMockTimer>();
    SetMockTimer(timer->AsWeakPtr());
    return std::move(timer);
  }

  std::unique_ptr<PageTimingSender> CreatePageTimingSender() override {
    return base::WrapUnique<PageTimingSender>(
        new FakePageTimingSender(&validator_));
  }

  void ExpectPageLoadTiming(const mojom::PageLoadTiming& timing) {
    SetFakePageLoadTiming(timing);
    validator_.ExpectPageLoadTiming(timing);
  }

  void SetFakePageLoadTiming(const mojom::PageLoadTiming& timing) {
    EXPECT_EQ(nullptr, fake_timing_.get());
    fake_timing_ = timing.Clone();
  }

  mojom::PageLoadTimingPtr GetTiming() const override {
    EXPECT_NE(nullptr, fake_timing_.get());
    return std::move(fake_timing_);
  }

  void VerifyExpectedTimings() const {
    EXPECT_EQ(nullptr, fake_timing_.get());
    validator_.VerifyExpectedTimings();
  }

  bool HasNoRenderFrame() const override { return false; }

 private:
  FakePageTimingSender::PageTimingValidator validator_;
  mutable mojom::PageLoadTimingPtr fake_timing_;
};

typedef testing::Test MetricsRenderFrameObserverTest;

TEST_F(MetricsRenderFrameObserverTest, NoMetrics) {
  TestMetricsRenderFrameObserver observer;
  observer.DidChangePerformanceTiming();
  ASSERT_EQ(nullptr, observer.GetMockTimer());
}

TEST_F(MetricsRenderFrameObserverTest, SingleMetric) {
  base::Time nav_start = base::Time::FromDoubleT(10);
  base::TimeDelta first_layout = base::TimeDelta::FromMillisecondsD(10);

  TestMetricsRenderFrameObserver observer;

  mojom::PageLoadTiming timing;
  page_load_metrics::InitPageLoadTimingForTest(&timing);
  timing.navigation_start = nav_start;
  observer.ExpectPageLoadTiming(timing);
  observer.DidCommitProvisionalLoad(true, false);
  observer.GetMockTimer()->Fire();

  timing.document_timing->first_layout = first_layout;
  observer.ExpectPageLoadTiming(timing);

  observer.DidChangePerformanceTiming();
  observer.GetMockTimer()->Fire();
}

TEST_F(MetricsRenderFrameObserverTest, MultipleMetrics) {
  base::Time nav_start = base::Time::FromDoubleT(10);
  base::TimeDelta first_layout = base::TimeDelta::FromMillisecondsD(2);
  base::TimeDelta dom_event = base::TimeDelta::FromMillisecondsD(2);
  base::TimeDelta load_event = base::TimeDelta::FromMillisecondsD(2);

  TestMetricsRenderFrameObserver observer;

  mojom::PageLoadTiming timing;
  page_load_metrics::InitPageLoadTimingForTest(&timing);
  timing.navigation_start = nav_start;
  observer.ExpectPageLoadTiming(timing);
  observer.DidCommitProvisionalLoad(true, false);
  observer.GetMockTimer()->Fire();

  timing.document_timing->first_layout = first_layout;
  timing.document_timing->dom_content_loaded_event_start = dom_event;
  observer.ExpectPageLoadTiming(timing);

  observer.DidChangePerformanceTiming();
  observer.GetMockTimer()->Fire();

  // At this point, we should have triggered the generation of two metrics.
  // Verify and reset the observer's expectations before moving on to the next
  // part of the test.
  observer.VerifyExpectedTimings();

  timing.document_timing->load_event_start = load_event;
  observer.ExpectPageLoadTiming(timing);

  observer.DidChangePerformanceTiming();
  observer.GetMockTimer()->Fire();

  // Verify and reset the observer's expectations before moving on to the next
  // part of the test.
  observer.VerifyExpectedTimings();

  // The PageLoadTiming above includes timing information for the first layout,
  // dom content, and load metrics. However, since we've already generated
  // timing information for all of these metrics previously, we do not expect
  // this invocation to generate any additional metrics.
  observer.SetFakePageLoadTiming(timing);
  observer.DidChangePerformanceTiming();
  ASSERT_FALSE(observer.GetMockTimer()->IsRunning());
}

TEST_F(MetricsRenderFrameObserverTest, MultipleNavigations) {
  base::Time nav_start = base::Time::FromDoubleT(10);
  base::TimeDelta first_layout = base::TimeDelta::FromMillisecondsD(2);
  base::TimeDelta dom_event = base::TimeDelta::FromMillisecondsD(2);
  base::TimeDelta load_event = base::TimeDelta::FromMillisecondsD(2);

  TestMetricsRenderFrameObserver observer;

  mojom::PageLoadTiming timing;
  page_load_metrics::InitPageLoadTimingForTest(&timing);
  timing.navigation_start = nav_start;
  observer.ExpectPageLoadTiming(timing);
  observer.DidCommitProvisionalLoad(true, false);
  observer.GetMockTimer()->Fire();

  timing.document_timing->first_layout = first_layout;
  timing.document_timing->dom_content_loaded_event_start = dom_event;
  timing.document_timing->load_event_start = load_event;
  observer.ExpectPageLoadTiming(timing);
  observer.DidChangePerformanceTiming();
  observer.GetMockTimer()->Fire();

  // At this point, we should have triggered the generation of two metrics.
  // Verify and reset the observer's expectations before moving on to the next
  // part of the test.
  observer.VerifyExpectedTimings();

  base::Time nav_start_2 = base::Time::FromDoubleT(100);
  base::TimeDelta first_layout_2 = base::TimeDelta::FromMillisecondsD(20);
  base::TimeDelta dom_event_2 = base::TimeDelta::FromMillisecondsD(20);
  base::TimeDelta load_event_2 = base::TimeDelta::FromMillisecondsD(20);
  mojom::PageLoadTiming timing_2;
  page_load_metrics::InitPageLoadTimingForTest(&timing_2);
  timing_2.navigation_start = nav_start_2;

  observer.SetMockTimer(nullptr);

  observer.ExpectPageLoadTiming(timing_2);
  observer.DidCommitProvisionalLoad(true, false);
  observer.GetMockTimer()->Fire();

  timing_2.document_timing->first_layout = first_layout_2;
  timing_2.document_timing->dom_content_loaded_event_start = dom_event_2;
  timing_2.document_timing->load_event_start = load_event_2;
  observer.ExpectPageLoadTiming(timing_2);

  observer.DidChangePerformanceTiming();
  observer.GetMockTimer()->Fire();
}

}  // namespace page_load_metrics
