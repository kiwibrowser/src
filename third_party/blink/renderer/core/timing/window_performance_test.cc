// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/timing/window_performance.h"
#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/bindings/core/v8/string_or_double.h"
#include "third_party/blink/renderer/bindings/core/v8/string_or_double_or_performance_measure_options.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_core.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/frame/performance_monitor.h"
#include "third_party/blink/renderer/core/loader/document_load_timing.h"
#include "third_party/blink/renderer/core/loader/document_loader.h"
#include "third_party/blink/renderer/core/testing/dummy_page_holder.h"
#include "third_party/blink/renderer/core/timing/dom_window_performance.h"
#include "third_party/blink/renderer/platform/testing/runtime_enabled_features_test_helpers.h"

namespace blink {

static const int kTimeOrigin = 500;

TimeTicks GetTimeOrigin() {
  return TimeTicks() + TimeDelta::FromSeconds(kTimeOrigin);
}

namespace {

class FakeTimer {
 public:
  FakeTimer(double init_time) {
    g_mock_time = init_time;
    original_time_function_ =
        WTF::SetTimeFunctionsForTesting(GetMockTimeInSeconds);
  }

  ~FakeTimer() { WTF::SetTimeFunctionsForTesting(original_time_function_); }

  static double GetMockTimeInSeconds() { return g_mock_time; }

  void AdvanceTimer(double duration) { g_mock_time += duration; }

 private:
  TimeFunction original_time_function_;
  static double g_mock_time;
};

double FakeTimer::g_mock_time = 1000.;

}  // namespace

class WindowPerformanceTest : public testing::Test {
 protected:
  void SetUp() override {
    ResetPerformance();

    // Create another dummy page holder and pretend this is the iframe.
    another_page_holder_ = DummyPageHolder::Create(IntSize(400, 300));
    another_page_holder_->GetDocument().SetURL(KURL("https://iframed.com/bar"));
  }

  bool ObservingLongTasks() {
    return PerformanceMonitor::InstrumentingMonitor(
        performance_->GetExecutionContext());
  }

  void AddLongTaskObserver() {
    // simulate with filter options.
    performance_->observer_filter_options_ |= PerformanceEntry::kLongTask;
  }

  void RemoveLongTaskObserver() {
    // simulate with filter options.
    performance_->observer_filter_options_ = PerformanceEntry::kInvalid;
  }

  void SimulateDidProcessLongTask() {
    auto* monitor = GetFrame()->GetPerformanceMonitor();
    monitor->WillExecuteScript(GetDocument());
    monitor->DidExecuteScript();
    monitor->DidProcessTask(0, 1);
  }

  void SimulateSwapPromise(TimeTicks timestamp) {
    performance_->ReportEventTimings(WebLayerTreeView::SwapResult::kDidSwap,
                                     timestamp);
  }

  LocalFrame* GetFrame() const { return &page_holder_->GetFrame(); }

  Document* GetDocument() const { return &page_holder_->GetDocument(); }

  LocalFrame* AnotherFrame() const { return &another_page_holder_->GetFrame(); }

  Document* AnotherDocument() const {
    return &another_page_holder_->GetDocument();
  }

  String SanitizedAttribution(ExecutionContext* context,
                              bool has_multiple_contexts,
                              LocalFrame* observer_frame) {
    return WindowPerformance::SanitizedAttribution(
               context, has_multiple_contexts, observer_frame)
        .first;
  }

  void ResetPerformance() {
    page_holder_ = DummyPageHolder::Create(IntSize(800, 600));
    page_holder_->GetDocument().SetURL(KURL("https://example.com"));
    performance_ =
        WindowPerformance::Create(page_holder_->GetDocument().domWindow());
    performance_->time_origin_ = GetTimeOrigin();
  }

  Persistent<WindowPerformance> performance_;
  std::unique_ptr<DummyPageHolder> page_holder_;
  std::unique_ptr<DummyPageHolder> another_page_holder_;
};

TEST_F(WindowPerformanceTest, LongTaskObserverInstrumentation) {
  performance_->UpdateLongTaskInstrumentation();
  EXPECT_FALSE(ObservingLongTasks());

  // Adding LongTask observer (with filer option) enables instrumentation.
  AddLongTaskObserver();
  performance_->UpdateLongTaskInstrumentation();
  EXPECT_TRUE(ObservingLongTasks());

  // Removing LongTask observer disables instrumentation.
  RemoveLongTaskObserver();
  performance_->UpdateLongTaskInstrumentation();
  EXPECT_FALSE(ObservingLongTasks());
}

TEST_F(WindowPerformanceTest, SanitizedLongTaskName) {
  // Unable to attribute, when no execution contents are available.
  EXPECT_EQ("unknown", SanitizedAttribution(nullptr, false, GetFrame()));

  // Attribute for same context (and same origin).
  EXPECT_EQ("self", SanitizedAttribution(GetDocument(), false, GetFrame()));

  // Unable to attribute, when multiple script execution contents are involved.
  EXPECT_EQ("multiple-contexts",
            SanitizedAttribution(GetDocument(), true, GetFrame()));
}

TEST_F(WindowPerformanceTest, SanitizedLongTaskName_CrossOrigin) {
  // Unable to attribute, when no execution contents are available.
  EXPECT_EQ("unknown", SanitizedAttribution(nullptr, false, GetFrame()));

  // Attribute for same context (and same origin).
  EXPECT_EQ("cross-origin-unreachable",
            SanitizedAttribution(AnotherDocument(), false, GetFrame()));
}

// https://crbug.com/706798: Checks that after navigation that have replaced the
// window object, calls to not garbage collected yet WindowPerformance belonging
// to the old window do not cause a crash.
TEST_F(WindowPerformanceTest, NavigateAway) {
  AddLongTaskObserver();
  performance_->UpdateLongTaskInstrumentation();
  EXPECT_TRUE(ObservingLongTasks());

  // Simulate navigation commit.
  DocumentInit init = DocumentInit::Create().WithFrame(GetFrame());
  GetDocument()->Shutdown();
  GetFrame()->SetDOMWindow(LocalDOMWindow::Create(*GetFrame()));
  GetFrame()->DomWindow()->InstallNewDocument(AtomicString(), init, false);

  // m_performance is still alive, and should not crash when notified.
  SimulateDidProcessLongTask();
}

// Checks that WindowPerformance object and its fields (like PerformanceTiming)
// function correctly after transition to another document in the same window.
// This happens when a page opens a new window and it navigates to a same-origin
// document.
TEST(PerformanceLifetimeTest, SurviveContextSwitch) {
  std::unique_ptr<DummyPageHolder> page_holder =
      DummyPageHolder::Create(IntSize(800, 600));

  WindowPerformance* perf =
      DOMWindowPerformance::performance(*page_holder->GetFrame().DomWindow());
  PerformanceTiming* timing = perf->timing();

  auto* document_loader = page_holder->GetFrame().Loader().GetDocumentLoader();
  ASSERT_TRUE(document_loader);
  document_loader->GetTiming().SetNavigationStart(CurrentTimeTicks());

  EXPECT_EQ(&page_holder->GetFrame(), perf->GetFrame());
  EXPECT_EQ(&page_holder->GetFrame(), timing->GetFrame());
  auto navigation_start = timing->navigationStart();
  EXPECT_NE(0U, navigation_start);

  // Simulate changing the document while keeping the window.
  page_holder->GetDocument().Shutdown();
  page_holder->GetFrame().DomWindow()->InstallNewDocument(
      AtomicString(),
      DocumentInit::Create().WithFrame(&page_holder->GetFrame()), false);

  EXPECT_EQ(perf, DOMWindowPerformance::performance(
                      *page_holder->GetFrame().DomWindow()));
  EXPECT_EQ(timing, perf->timing());
  EXPECT_EQ(&page_holder->GetFrame(), perf->GetFrame());
  EXPECT_EQ(&page_holder->GetFrame(), timing->GetFrame());
  EXPECT_EQ(navigation_start, timing->navigationStart());
}

// Make sure the output entries with the same timestamps follow the insertion
// order. (http://crbug.com/767560)
TEST_F(WindowPerformanceTest, EnsureEntryListOrder) {
  V8TestingScope scope;
  FakeTimer timer(kTimeOrigin);

  DummyExceptionStateForTesting exception_state;
  timer.AdvanceTimer(2);
  for (int i = 0; i < 8; i++) {
    performance_->mark(scope.GetScriptState(), String::Number(i),
                       exception_state);
  }
  timer.AdvanceTimer(2);
  for (int i = 8; i < 17; i++) {
    performance_->mark(scope.GetScriptState(), String::Number(i),
                       exception_state);
  }
  PerformanceEntryVector entries = performance_->getEntries();
  EXPECT_EQ(17U, entries.size());
  for (int i = 0; i < 8; i++) {
    EXPECT_EQ(String::Number(i), entries[i]->name());
    EXPECT_NEAR(2000, entries[i]->startTime(), 0.005);
  }
  for (int i = 8; i < 17; i++) {
    EXPECT_EQ(String::Number(i), entries[i]->name());
    EXPECT_NEAR(4000, entries[i]->startTime(), 0.005);
  }
}

TEST_F(WindowPerformanceTest, EventTimingBeforeOnLoad) {
  ScopedEventTimingForTest event_timing(true);
  EXPECT_TRUE(page_holder_->GetFrame().Loader().GetDocumentLoader());

  TimeTicks start_time = TimeTicksFromSeconds(kTimeOrigin + 1.1);
  TimeTicks processing_start = TimeTicksFromSeconds(kTimeOrigin + 3.3);
  TimeTicks processing_end = TimeTicksFromSeconds(kTimeOrigin + 3.8);
  performance_->RegisterEventTiming("click", start_time, processing_start,
                                    processing_end, false);
  TimeTicks swap_time = TimeTicksFromSeconds(kTimeOrigin + 6.0);
  SimulateSwapPromise(swap_time);
  EXPECT_EQ(1u, performance_->getEntriesByName("click", "event").size());
  performance_->clearEventTimings();

  page_holder_->GetFrame()
      .Loader()
      .GetDocumentLoader()
      ->GetTiming()
      .MarkLoadEventStart();
  performance_->RegisterEventTiming("click", start_time, processing_start,
                                    processing_end, true);
  SimulateSwapPromise(swap_time);
  EXPECT_EQ(0u, performance_->getEntriesByName("click", "event").size());
  performance_->clearEventTimings();

  EXPECT_TRUE(page_holder_->GetFrame().Loader().GetDocumentLoader());
  GetFrame()->PrepareForCommit();
  EXPECT_FALSE(page_holder_->GetFrame().Loader().GetDocumentLoader());
  performance_->RegisterEventTiming("click", start_time, processing_start,
                                    processing_end, false);
  SimulateSwapPromise(swap_time);
  EXPECT_EQ(1u, performance_->getEntriesByName("click", "event").size());
  performance_->clearEventTimings();
}

TEST_F(WindowPerformanceTest, EventTimingDuration) {
  ScopedEventTimingForTest event_timing(true);

  TimeTicks start_time = TimeTicksFromSeconds(kTimeOrigin + 1.0);
  TimeTicks processing_start = TimeTicksFromSeconds(kTimeOrigin + 1.001);
  TimeTicks processing_end = TimeTicksFromSeconds(kTimeOrigin + 1.002);
  performance_->RegisterEventTiming("click", start_time, processing_start,
                                    processing_end, false);
  TimeTicks short_swap_time = TimeTicksFromSeconds(kTimeOrigin + 1.003);
  SimulateSwapPromise(short_swap_time);
  EXPECT_EQ(0u, performance_->getEntriesByName("click", "event").size());

  performance_->RegisterEventTiming("click", start_time, processing_start,
                                    processing_end, true);
  TimeTicks long_swap_time = TimeTicksFromSeconds(kTimeOrigin + 1.1);
  SimulateSwapPromise(long_swap_time);
  EXPECT_EQ(1u, performance_->getEntriesByName("click", "event").size());

  performance_->RegisterEventTiming("click", start_time, processing_start,
                                    processing_end, true);
  SimulateSwapPromise(short_swap_time);
  performance_->RegisterEventTiming("click", start_time, processing_start,
                                    processing_end, false);
  SimulateSwapPromise(long_swap_time);
  EXPECT_EQ(2u, performance_->getEntriesByName("click", "event").size());
}

TEST_F(WindowPerformanceTest, MultipleEventsSameSwap) {
  ScopedEventTimingForTest event_timing(true);

  size_t num_events = 10;
  for (size_t i = 0; i < num_events; ++i) {
    TimeTicks start_time = TimeTicksFromSeconds(kTimeOrigin + i);
    TimeTicks processing_start = TimeTicksFromSeconds(kTimeOrigin + i + 0.1);
    TimeTicks processing_end = TimeTicksFromSeconds(kTimeOrigin + i + 0.2);
    performance_->RegisterEventTiming("click", start_time, processing_start,
                                      processing_end, false);
    EXPECT_EQ(0u, performance_->getEntriesByName("click", "event").size());
  }
  TimeTicks swap_time = TimeTicksFromSeconds(kTimeOrigin + num_events);
  SimulateSwapPromise(swap_time);
  EXPECT_EQ(num_events,
            performance_->getEntriesByName("click", "event").size());
}

// Test for existence of 'firstInput' given different types of first events.
TEST_F(WindowPerformanceTest, FirstInput) {
  struct {
    String event_type;
    bool should_report;
  } inputs[] = {{"click", true},     {"keydown", true},
                {"keypress", false}, {"pointerdown", false},
                {"mousedown", true}, {"mousemove", false},
                {"mouseover", false}};
  for (const auto& input : inputs) {
    // firstInput does not have a |duration| threshold so use close values.
    performance_->RegisterEventTiming(
        input.event_type, GetTimeOrigin(),
        GetTimeOrigin() + TimeDelta::FromMilliseconds(1),
        GetTimeOrigin() + TimeDelta::FromMilliseconds(2), false);
    SimulateSwapPromise(GetTimeOrigin() + TimeDelta::FromMilliseconds(3));
    PerformanceEntryVector firstInputs =
        performance_->getEntriesByType("firstInput");
    EXPECT_GE(1u, firstInputs.size());
    EXPECT_EQ(input.should_report, firstInputs.size() == 1u);
    ResetPerformance();
  }
}

// Test that the 'firstInput' is populated after some irrelevant events are
// ignored.
TEST_F(WindowPerformanceTest, FirstInputAfterIgnored) {
  String several_events[] = {"mousemove", "mouseover", "mousedown"};
  for (const auto& event : several_events) {
    performance_->RegisterEventTiming(
        event, GetTimeOrigin(),
        GetTimeOrigin() + TimeDelta::FromMilliseconds(1),
        GetTimeOrigin() + TimeDelta::FromMilliseconds(2), false);
  }
  SimulateSwapPromise(GetTimeOrigin() + TimeDelta::FromMilliseconds(3));
  ASSERT_EQ(1u, performance_->getEntriesByType("firstInput").size());
  EXPECT_EQ("mousedown",
            performance_->getEntriesByType("firstInput")[0]->name());
}

// Test that pointerdown followed by pointerup works as a 'firstInput'.
TEST_F(WindowPerformanceTest, FirstPointerUp) {
  TimeTicks start_time = GetTimeOrigin();
  TimeTicks processing_start = GetTimeOrigin() + TimeDelta::FromMilliseconds(1);
  TimeTicks processing_end = GetTimeOrigin() + TimeDelta::FromMilliseconds(2);
  TimeTicks swap_time = GetTimeOrigin() + TimeDelta::FromMilliseconds(3);
  performance_->RegisterEventTiming("pointerdown", start_time, processing_start,
                                    processing_end, false);
  SimulateSwapPromise(swap_time);
  EXPECT_EQ(0u, performance_->getEntriesByType("firstInput").size());
  performance_->RegisterEventTiming("pointerup", start_time, processing_start,
                                    processing_end, false);
  SimulateSwapPromise(swap_time);
  EXPECT_EQ(1u, performance_->getEntriesByType("firstInput").size());
  // The name of the entry should be "pointerdown".
  EXPECT_EQ(1u,
            performance_->getEntriesByName("pointerdown", "firstInput").size());
}

}  // namespace blink
