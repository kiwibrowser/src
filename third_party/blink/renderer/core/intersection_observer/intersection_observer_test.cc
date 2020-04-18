// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/intersection_observer/intersection_observer.h"

#include "third_party/blink/renderer/core/exported/web_view_impl.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/intersection_observer/intersection_observer_delegate.h"
#include "third_party/blink/renderer/core/intersection_observer/intersection_observer_init.h"
#include "third_party/blink/renderer/core/testing/sim/sim_compositor.h"
#include "third_party/blink/renderer/core/testing/sim/sim_request.h"
#include "third_party/blink/renderer/core/testing/sim/sim_test.h"
#include "third_party/blink/renderer/platform/geometry/float_rect.h"
#include "third_party/blink/renderer/platform/testing/runtime_enabled_features_test_helpers.h"
#include "third_party/blink/renderer/platform/testing/unit_test_helpers.h"
#include "third_party/blink/renderer/platform/wtf/time.h"

namespace blink {

namespace {

class TestIntersectionObserverDelegate : public IntersectionObserverDelegate {
 public:
  TestIntersectionObserverDelegate(Document& document)
      : document_(document), call_count_(0) {}
  void Deliver(const HeapVector<Member<IntersectionObserverEntry>>& entry,
               IntersectionObserver&) override {
    call_count_++;

    if (!entry.back()->intersectionRect()) {
      last_intersection_rect_ = FloatRect();
    } else {
      last_intersection_rect_ =
          FloatRect(entry.back()->intersectionRect()->x(),
                    entry.back()->intersectionRect()->y(),
                    entry.back()->intersectionRect()->width(),
                    entry.back()->intersectionRect()->height());
    }
  }
  ExecutionContext* GetExecutionContext() const override { return document_; }
  int CallCount() const { return call_count_; }
  FloatRect LastIntersectionRect() const { return last_intersection_rect_; }

  void Trace(blink::Visitor* visitor) override {
    IntersectionObserverDelegate::Trace(visitor);
    visitor->Trace(document_);
  }

 private:
  FloatRect last_intersection_rect_;
  Member<Document> document_;
  int call_count_;
};

}  // namespace

class IntersectionObserverTest
    : public testing::WithParamInterface<bool>,
      public SimTest,
      ScopedIntersectionObserverGeometryMapperForTest {
 public:
  IntersectionObserverTest()
      : ScopedIntersectionObserverGeometryMapperForTest(GetParam()) {}
};

INSTANTIATE_TEST_CASE_P(All, IntersectionObserverTest, testing::Bool());

TEST_P(IntersectionObserverTest, ObserveSchedulesFrame) {
  SimRequest main_resource("https://example.com/", "text/html");
  LoadURL("https://example.com/");
  main_resource.Complete("<div id='target'></div>");

  IntersectionObserverInit observer_init;
  DummyExceptionStateForTesting exception_state;
  TestIntersectionObserverDelegate* observer_delegate =
      new TestIntersectionObserverDelegate(GetDocument());
  IntersectionObserver* observer = IntersectionObserver::Create(
      observer_init, *observer_delegate, exception_state);
  ASSERT_FALSE(exception_state.HadException());

  Compositor().BeginFrame();
  ASSERT_FALSE(Compositor().NeedsBeginFrame());
  EXPECT_TRUE(observer->takeRecords(exception_state).IsEmpty());
  EXPECT_EQ(observer_delegate->CallCount(), 0);

  Element* target = GetDocument().getElementById("target");
  ASSERT_TRUE(target);
  observer->observe(target, exception_state);
  EXPECT_TRUE(Compositor().NeedsBeginFrame());
}

TEST_P(IntersectionObserverTest, ResumePostsTask) {
  WebView().Resize(WebSize(800, 600));
  SimRequest main_resource("https://example.com/", "text/html");
  LoadURL("https://example.com/");
  main_resource.Complete(R"HTML(
    <div id='leading-space' style='height: 700px;'></div>
    <div id='target'></div>
    <div id='trailing-space' style='height: 700px;'></div>
  )HTML");

  IntersectionObserverInit observer_init;
  DummyExceptionStateForTesting exception_state;
  TestIntersectionObserverDelegate* observer_delegate =
      new TestIntersectionObserverDelegate(GetDocument());
  IntersectionObserver* observer = IntersectionObserver::Create(
      observer_init, *observer_delegate, exception_state);
  ASSERT_FALSE(exception_state.HadException());

  Element* target = GetDocument().getElementById("target");
  ASSERT_TRUE(target);
  observer->observe(target, exception_state);

  Compositor().BeginFrame();
  test::RunPendingTasks();
  EXPECT_EQ(observer_delegate->CallCount(), 1);

  // When document is not suspended, beginFrame() will generate notifications
  // and post a task to deliver them.
  GetDocument().View()->LayoutViewportScrollableArea()->SetScrollOffset(
      ScrollOffset(0, 300), kProgrammaticScroll);
  Compositor().BeginFrame();
  EXPECT_EQ(observer_delegate->CallCount(), 1);
  test::RunPendingTasks();
  EXPECT_EQ(observer_delegate->CallCount(), 2);

  // When a document is suspended, beginFrame() will generate a notification,
  // but it will not be delivered.  The notification will, however, be
  // available via takeRecords();
  GetDocument().PauseScheduledTasks();
  GetDocument().View()->LayoutViewportScrollableArea()->SetScrollOffset(
      ScrollOffset(0, 0), kProgrammaticScroll);
  Compositor().BeginFrame();
  EXPECT_EQ(observer_delegate->CallCount(), 2);
  test::RunPendingTasks();
  EXPECT_EQ(observer_delegate->CallCount(), 2);
  EXPECT_FALSE(observer->takeRecords(exception_state).IsEmpty());

  // Generate a notification while document is suspended; then resume document.
  // Notification should happen in a post task.
  GetDocument().View()->LayoutViewportScrollableArea()->SetScrollOffset(
      ScrollOffset(0, 300), kProgrammaticScroll);
  Compositor().BeginFrame();
  test::RunPendingTasks();
  EXPECT_EQ(observer_delegate->CallCount(), 2);
  GetDocument().UnpauseScheduledTasks();
  EXPECT_EQ(observer_delegate->CallCount(), 2);
  test::RunPendingTasks();
  EXPECT_EQ(observer_delegate->CallCount(), 3);
}

TEST_P(IntersectionObserverTest, DisconnectClearsNotifications) {
  WebView().Resize(WebSize(800, 600));
  SimRequest main_resource("https://example.com/", "text/html");
  LoadURL("https://example.com/");
  main_resource.Complete(R"HTML(
    <div id='leading-space' style='height: 700px;'></div>
    <div id='target'></div>
    <div id='trailing-space' style='height: 700px;'></div>
  )HTML");

  IntersectionObserverInit observer_init;
  DummyExceptionStateForTesting exception_state;
  TestIntersectionObserverDelegate* observer_delegate =
      new TestIntersectionObserverDelegate(GetDocument());
  IntersectionObserver* observer = IntersectionObserver::Create(
      observer_init, *observer_delegate, exception_state);
  ASSERT_FALSE(exception_state.HadException());

  Element* target = GetDocument().getElementById("target");
  ASSERT_TRUE(target);
  observer->observe(target, exception_state);

  Compositor().BeginFrame();
  test::RunPendingTasks();
  EXPECT_EQ(observer_delegate->CallCount(), 1);

  // If disconnect() is called while an observer has unsent notifications,
  // those notifications should be discarded.
  GetDocument().View()->LayoutViewportScrollableArea()->SetScrollOffset(
      ScrollOffset(0, 300), kProgrammaticScroll);
  Compositor().BeginFrame();
  observer->disconnect();
  test::RunPendingTasks();
  EXPECT_EQ(observer_delegate->CallCount(), 1);
}

TEST_P(IntersectionObserverTest, RootIntersectionWithForceZeroLayoutHeight) {
  WebView().GetSettings()->SetForceZeroLayoutHeight(true);
  WebView().Resize(WebSize(800, 600));
  SimRequest main_resource("https://example.com/", "text/html");
  LoadURL("https://example.com/");
  main_resource.Complete(R"HTML(
    <!DOCTYPE html>
    <style>
      body {
        margin: 0;
        height: 2000px;
      }

      #target {
        width: 100px;
        height: 100px;
        position: absolute;
        top: 1000px;
        left: 200px;
      }
    </style>
    <div id='target'></div>
  )HTML");

  IntersectionObserverInit observer_init;
  DummyExceptionStateForTesting exception_state;
  TestIntersectionObserverDelegate* observer_delegate =
      new TestIntersectionObserverDelegate(GetDocument());
  IntersectionObserver* observer = IntersectionObserver::Create(
      observer_init, *observer_delegate, exception_state);
  ASSERT_FALSE(exception_state.HadException());

  Element* target = GetDocument().getElementById("target");
  ASSERT_TRUE(target);
  observer->observe(target, exception_state);

  Compositor().BeginFrame();
  test::RunPendingTasks();
  ASSERT_EQ(observer_delegate->CallCount(), 1);
  EXPECT_TRUE(observer_delegate->LastIntersectionRect().IsEmpty());

  GetDocument().View()->LayoutViewportScrollableArea()->SetScrollOffset(
      ScrollOffset(0, 600), kProgrammaticScroll);
  Compositor().BeginFrame();
  test::RunPendingTasks();
  ASSERT_EQ(observer_delegate->CallCount(), 2);
  EXPECT_FALSE(observer_delegate->LastIntersectionRect().IsEmpty());
  EXPECT_EQ(FloatRect(200, 400, 100, 100),
            observer_delegate->LastIntersectionRect());

  GetDocument().View()->LayoutViewportScrollableArea()->SetScrollOffset(
      ScrollOffset(0, 1200), kProgrammaticScroll);
  Compositor().BeginFrame();
  test::RunPendingTasks();
  ASSERT_EQ(observer_delegate->CallCount(), 3);
  EXPECT_TRUE(observer_delegate->LastIntersectionRect().IsEmpty());
}

TEST_P(IntersectionObserverTest, TrackVisibilityInit) {
  ScopedIntersectionObserverV2ForTest iov2_enabled(true);
  IntersectionObserverInit observer_init;
  DummyExceptionStateForTesting exception_state;
  TestIntersectionObserverDelegate* observer_delegate =
      new TestIntersectionObserverDelegate(GetDocument());
  IntersectionObserver* observer = IntersectionObserver::Create(
      observer_init, *observer_delegate, exception_state);
  EXPECT_FALSE(observer->trackVisibility());
  observer_init.setTrackVisibility(true);
  observer = IntersectionObserver::Create(observer_init, *observer_delegate,
                                          exception_state);
  EXPECT_TRUE(observer->trackVisibility());
}

}  // namespace blink
