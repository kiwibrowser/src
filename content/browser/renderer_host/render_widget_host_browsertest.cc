// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <vector>

#include "base/optional.h"
#include "base/run_loop.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "components/viz/common/features.h"
#include "components/viz/common/frame_sinks/copy_output_request.h"
#include "components/viz/common/frame_sinks/copy_output_result.h"
#include "components/viz/common/quads/compositor_frame.h"
#include "components/viz/common/quads/render_pass.h"
#include "components/viz/common/surfaces/local_surface_id.h"
#include "content/browser/bad_message.h"
#include "content/browser/renderer_host/input/touch_emulator.h"
#include "content/browser/renderer_host/render_widget_host_impl.h"
#include "content/browser/renderer_host/render_widget_host_input_event_router.h"
#include "content/browser/renderer_host/render_widget_host_view_base.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/common/input/synthetic_web_input_event_builders.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/shell/browser/shell.h"
#include "content/test/content_browser_test_utils_internal.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/web_input_event.h"
#include "third_party/blink/public/platform/web_mouse_event.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/events/base_event_utils.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/latency/latency_info.h"

namespace content {

class RenderWidgetHostBrowserTest : public ContentBrowserTest {};

IN_PROC_BROWSER_TEST_F(RenderWidgetHostBrowserTest,
                       ProhibitsCopyRequestsFromRenderer) {
  NavigateToURL(shell(), GURL("data:text/html,<!doctype html>"
                              "<body style='background-color: red;'></body>"));

  // Wait for the view's surface to become available.
  auto* const view = static_cast<RenderWidgetHostViewBase*>(
      shell()->web_contents()->GetRenderWidgetHostView());
  while (!view->IsSurfaceAvailableForCopy()) {
    base::RunLoop run_loop;
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE, run_loop.QuitClosure(),
        base::TimeDelta::FromMilliseconds(250));
    run_loop.Run();
  }

  // The browser process should be allowed to make a CopyOutputRequest.
  bool did_receive_copy_result = false;
  base::RunLoop run_loop;
  view->CopyFromSurface(gfx::Rect(), gfx::Size(),
                        base::BindOnce(
                            [](bool* success, base::OnceClosure quit_closure,
                               const SkBitmap& bitmap) {
                              *success = !bitmap.drawsNothing();
                              std::move(quit_closure).Run();
                            },
                            &did_receive_copy_result, run_loop.QuitClosure()));
  run_loop.Run();
  ASSERT_TRUE(did_receive_copy_result);

  // Create a simulated-from-renderer CompositorFrame with a CopyOutputRequest.
  viz::CompositorFrame frame;
  std::unique_ptr<viz::RenderPass> pass = viz::RenderPass::Create();
  const gfx::Rect output_rect =
      gfx::Rect(view->GetCompositorViewportPixelSize());
  pass->SetNew(1 /* render pass id */, output_rect, output_rect,
               gfx::Transform());
  bool did_receive_aborted_copy_result = false;
  pass->copy_requests.push_back(std::make_unique<viz::CopyOutputRequest>(
      viz::CopyOutputRequest::ResultFormat::RGBA_BITMAP,
      base::BindOnce(
          [](bool* got_nothing, std::unique_ptr<viz::CopyOutputResult> result) {
            *got_nothing = result->IsEmpty();
          },
          &did_receive_aborted_copy_result)));
  frame.render_pass_list.push_back(std::move(pass));

  // Submit the frame and expect the renderer to be instantly killed.
  auto* const host = RenderWidgetHostImpl::From(view->GetRenderWidgetHost());
  RenderProcessHostKillWaiter waiter(host->GetProcess());
  host->SubmitCompositorFrame(viz::LocalSurfaceId(), std::move(frame),
                              base::nullopt, 0);
  base::Optional<bad_message::BadMessageReason> result = waiter.Wait();
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(bad_message::RWH_COPY_REQUEST_ATTEMPT, *result);

  // Check that the copy request result callback received an empty result. In a
  // normal browser, the requestor (in the render process) might never see a
  // response to the copy request before the process is killed. Nevertheless,
  // ensure the result is empty, just in case there is a race.
  EXPECT_TRUE(did_receive_aborted_copy_result);
}

class TestInputEventObserver : public RenderWidgetHost::InputEventObserver {
 public:
  using EventTypeVector = std::vector<blink::WebInputEvent::Type>;

  ~TestInputEventObserver() override {}

  void OnInputEvent(const blink::WebInputEvent& event) override {
    dispatched_events_.push_back(event.GetType());
  }

  void OnInputEventAck(InputEventAckSource source,
                       InputEventAckState state,
                       const blink::WebInputEvent& event) override {
    if (blink::WebInputEvent::IsTouchEventType(event.GetType()))
      acked_touch_event_type_ = event.GetType();
  }

  EventTypeVector GetAndResetDispatchedEventTypes() {
    EventTypeVector new_event_types;
    std::swap(new_event_types, dispatched_events_);
    return new_event_types;
  }

  blink::WebInputEvent::Type acked_touch_event_type() const {
    return acked_touch_event_type_;
  }

 private:
  EventTypeVector dispatched_events_;
  blink::WebInputEvent::Type acked_touch_event_type_;
};

class RenderWidgetHostTouchEmulatorBrowserTest
    : public RenderWidgetHostBrowserTest {
 public:
  RenderWidgetHostTouchEmulatorBrowserTest()
      : view_(nullptr),
        host_(nullptr),
        router_(nullptr),
        last_simulated_event_time_(ui::EventTimeForNow()),
        simulated_event_time_delta_(base::TimeDelta::FromMilliseconds(100)) {}

  void SetUpOnMainThread() override {
    RenderWidgetHostBrowserTest::SetUpOnMainThread();

    NavigateToURL(shell(),
                  GURL("data:text/html,<!doctype html>"
                       "<body style='background-color: red;'></body>"));

    view_ = static_cast<RenderWidgetHostViewBase*>(
        shell()->web_contents()->GetRenderWidgetHostView());
    host_ = static_cast<RenderWidgetHostImpl*>(view_->GetRenderWidgetHost());
    router_ = static_cast<WebContentsImpl*>(shell()->web_contents())
                  ->GetInputEventRouter();
    ASSERT_TRUE(router_);
  }

  base::TimeTicks GetNextSimulatedEventTime() {
    last_simulated_event_time_ += simulated_event_time_delta_;
    return last_simulated_event_time_;
  }

  void SimulateRoutedMouseEvent(blink::WebInputEvent::Type type,
                                int x,
                                int y,
                                int modifiers,
                                bool pressed) {
    blink::WebMouseEvent event =
        SyntheticWebMouseEventBuilder::Build(type, x, y, modifiers);
    if (pressed)
      event.button = blink::WebMouseEvent::Button::kLeft;
    event.SetTimeStamp(GetNextSimulatedEventTime());
    router_->RouteMouseEvent(view_, &event, ui::LatencyInfo());
  }

  RenderWidgetHostImpl* host() { return host_; }

 private:
  RenderWidgetHostViewBase* view_;
  RenderWidgetHostImpl* host_;
  RenderWidgetHostInputEventRouter* router_;

  base::TimeTicks last_simulated_event_time_;
  base::TimeDelta simulated_event_time_delta_;
};

IN_PROC_BROWSER_TEST_F(RenderWidgetHostTouchEmulatorBrowserTest,
                       TouchEmulator) {
  // All touches will be immediately acked instead of sending them to the
  // renderer since the test page does not have a touch handler.
  host()->GetTouchEmulator()->Enable(
      TouchEmulator::Mode::kEmulatingTouchFromMouse,
      ui::GestureProviderConfigType::GENERIC_MOBILE);

  TestInputEventObserver observer;
  host()->AddInputEventObserver(&observer);

  SimulateRoutedMouseEvent(blink::WebInputEvent::kMouseMove, 10, 10, 0, false);
  TestInputEventObserver::EventTypeVector dispatched_events =
      observer.GetAndResetDispatchedEventTypes();
  EXPECT_EQ(0u, dispatched_events.size());

  // Mouse press becomes touch start which in turn becomes tap.
  SimulateRoutedMouseEvent(blink::WebInputEvent::kMouseDown, 10, 10, 0, true);
  EXPECT_EQ(blink::WebInputEvent::kTouchStart,
            observer.acked_touch_event_type());
  dispatched_events = observer.GetAndResetDispatchedEventTypes();
  ASSERT_EQ(2u, dispatched_events.size());
  EXPECT_EQ(blink::WebInputEvent::kTouchStart, dispatched_events[0]);
  EXPECT_EQ(blink::WebInputEvent::kGestureTapDown, dispatched_events[1]);

  // Mouse drag generates touch move, cancels tap and starts scroll.
  SimulateRoutedMouseEvent(blink::WebInputEvent::kMouseMove, 10, 30, 0, true);
  dispatched_events = observer.GetAndResetDispatchedEventTypes();
  ASSERT_EQ(4u, dispatched_events.size());
  EXPECT_EQ(blink::WebInputEvent::kTouchMove, dispatched_events[0]);
  EXPECT_EQ(blink::WebInputEvent::kGestureTapCancel, dispatched_events[1]);
  EXPECT_EQ(blink::WebInputEvent::kGestureScrollBegin, dispatched_events[2]);
  EXPECT_EQ(blink::WebInputEvent::kGestureScrollUpdate, dispatched_events[3]);
  EXPECT_EQ(blink::WebInputEvent::kTouchMove,
            observer.acked_touch_event_type());
  EXPECT_EQ(0u, observer.GetAndResetDispatchedEventTypes().size());

  // Mouse drag with shift becomes pinch.
  SimulateRoutedMouseEvent(blink::WebInputEvent::kMouseMove, 10, 35,
                           blink::WebInputEvent::kShiftKey, true);
  EXPECT_EQ(blink::WebInputEvent::kTouchMove,
            observer.acked_touch_event_type());

  dispatched_events = observer.GetAndResetDispatchedEventTypes();
  ASSERT_EQ(2u, dispatched_events.size());
  EXPECT_EQ(blink::WebInputEvent::kTouchMove, dispatched_events[0]);
  EXPECT_EQ(blink::WebInputEvent::kGesturePinchBegin, dispatched_events[1]);

  SimulateRoutedMouseEvent(blink::WebInputEvent::kMouseMove, 10, 50,
                           blink::WebInputEvent::kShiftKey, true);
  EXPECT_EQ(blink::WebInputEvent::kTouchMove,
            observer.acked_touch_event_type());

  dispatched_events = observer.GetAndResetDispatchedEventTypes();
  ASSERT_EQ(2u, dispatched_events.size());
  EXPECT_EQ(blink::WebInputEvent::kTouchMove, dispatched_events[0]);
  EXPECT_EQ(blink::WebInputEvent::kGesturePinchUpdate, dispatched_events[1]);

  // Mouse drag without shift becomes scroll again.
  SimulateRoutedMouseEvent(blink::WebInputEvent::kMouseMove, 10, 60, 0, true);
  EXPECT_EQ(blink::WebInputEvent::kTouchMove,
            observer.acked_touch_event_type());

  dispatched_events = observer.GetAndResetDispatchedEventTypes();
  ASSERT_EQ(3u, dispatched_events.size());
  EXPECT_EQ(blink::WebInputEvent::kTouchMove, dispatched_events[0]);
  EXPECT_EQ(blink::WebInputEvent::kGesturePinchEnd, dispatched_events[1]);
  EXPECT_EQ(blink::WebInputEvent::kGestureScrollUpdate, dispatched_events[2]);

  SimulateRoutedMouseEvent(blink::WebInputEvent::kMouseMove, 10, 70, 0, true);
  EXPECT_EQ(blink::WebInputEvent::kTouchMove,
            observer.acked_touch_event_type());
  dispatched_events = observer.GetAndResetDispatchedEventTypes();
  ASSERT_EQ(2u, dispatched_events.size());
  EXPECT_EQ(blink::WebInputEvent::kTouchMove, dispatched_events[0]);
  EXPECT_EQ(blink::WebInputEvent::kGestureScrollUpdate, dispatched_events[1]);

  SimulateRoutedMouseEvent(blink::WebInputEvent::kMouseUp, 10, 70, 0, true);
  EXPECT_EQ(blink::WebInputEvent::kTouchEnd, observer.acked_touch_event_type());
  dispatched_events = observer.GetAndResetDispatchedEventTypes();
  ASSERT_EQ(2u, dispatched_events.size());
  EXPECT_EQ(blink::WebInputEvent::kTouchEnd, dispatched_events[0]);
  EXPECT_EQ(blink::WebInputEvent::kGestureScrollEnd, dispatched_events[1]);

  // Mouse move does nothing.
  SimulateRoutedMouseEvent(blink::WebInputEvent::kMouseMove, 10, 80, 0, false);
  dispatched_events = observer.GetAndResetDispatchedEventTypes();
  EXPECT_EQ(0u, dispatched_events.size());

  // Another mouse down continues scroll.
  SimulateRoutedMouseEvent(blink::WebInputEvent::kMouseDown, 10, 80, 0, true);
  EXPECT_EQ(blink::WebInputEvent::kTouchStart,
            observer.acked_touch_event_type());
  dispatched_events = observer.GetAndResetDispatchedEventTypes();
  ASSERT_EQ(2u, dispatched_events.size());
  EXPECT_EQ(blink::WebInputEvent::kTouchStart, dispatched_events[0]);
  EXPECT_EQ(blink::WebInputEvent::kGestureTapDown, dispatched_events[1]);
  SimulateRoutedMouseEvent(blink::WebInputEvent::kMouseMove, 10, 100, 0, true);
  EXPECT_EQ(blink::WebInputEvent::kTouchMove,
            observer.acked_touch_event_type());
  dispatched_events = observer.GetAndResetDispatchedEventTypes();
  ASSERT_EQ(4u, dispatched_events.size());
  EXPECT_EQ(blink::WebInputEvent::kTouchMove, dispatched_events[0]);
  EXPECT_EQ(blink::WebInputEvent::kGestureTapCancel, dispatched_events[1]);
  EXPECT_EQ(blink::WebInputEvent::kGestureScrollBegin, dispatched_events[2]);
  EXPECT_EQ(blink::WebInputEvent::kGestureScrollUpdate, dispatched_events[3]);
  EXPECT_EQ(0u, observer.GetAndResetDispatchedEventTypes().size());

  // Another pinch.
  SimulateRoutedMouseEvent(blink::WebInputEvent::kMouseMove, 10, 110,
                           blink::WebInputEvent::kShiftKey, true);
  EXPECT_EQ(blink::WebInputEvent::kTouchMove,
            observer.acked_touch_event_type());
  dispatched_events = observer.GetAndResetDispatchedEventTypes();
  EXPECT_EQ(2u, dispatched_events.size());
  EXPECT_EQ(blink::WebInputEvent::kTouchMove, dispatched_events[0]);
  EXPECT_EQ(blink::WebInputEvent::kGesturePinchBegin, dispatched_events[1]);
  SimulateRoutedMouseEvent(blink::WebInputEvent::kMouseMove, 10, 120,
                           blink::WebInputEvent::kShiftKey, true);
  EXPECT_EQ(blink::WebInputEvent::kTouchMove,
            observer.acked_touch_event_type());
  dispatched_events = observer.GetAndResetDispatchedEventTypes();
  EXPECT_EQ(2u, dispatched_events.size());
  EXPECT_EQ(blink::WebInputEvent::kTouchMove, dispatched_events[0]);
  EXPECT_EQ(blink::WebInputEvent::kGesturePinchUpdate, dispatched_events[1]);

  // Turn off emulation during a pinch.
  host()->GetTouchEmulator()->Disable();
  EXPECT_EQ(blink::WebInputEvent::kTouchCancel,
            observer.acked_touch_event_type());
  dispatched_events = observer.GetAndResetDispatchedEventTypes();
  ASSERT_EQ(3u, dispatched_events.size());
  EXPECT_EQ(blink::WebInputEvent::kTouchCancel, dispatched_events[0]);
  EXPECT_EQ(blink::WebInputEvent::kGesturePinchEnd, dispatched_events[1]);
  EXPECT_EQ(blink::WebInputEvent::kGestureScrollEnd, dispatched_events[2]);

  // Mouse event should pass untouched.
  SimulateRoutedMouseEvent(blink::WebInputEvent::kMouseMove, 10, 10,
                           blink::WebInputEvent::kShiftKey, true);
  dispatched_events = observer.GetAndResetDispatchedEventTypes();
  ASSERT_EQ(1u, dispatched_events.size());
  EXPECT_EQ(blink::WebInputEvent::kMouseMove, dispatched_events[0]);

  // Turn on emulation.
  host()->GetTouchEmulator()->Enable(
      TouchEmulator::Mode::kEmulatingTouchFromMouse,
      ui::GestureProviderConfigType::GENERIC_MOBILE);

  // Another touch.
  SimulateRoutedMouseEvent(blink::WebInputEvent::kMouseDown, 10, 10, 0, true);
  EXPECT_EQ(blink::WebInputEvent::kTouchStart,
            observer.acked_touch_event_type());
  dispatched_events = observer.GetAndResetDispatchedEventTypes();
  ASSERT_EQ(2u, dispatched_events.size());
  EXPECT_EQ(blink::WebInputEvent::kTouchStart, dispatched_events[0]);
  EXPECT_EQ(blink::WebInputEvent::kGestureTapDown, dispatched_events[1]);

  // Scroll.
  SimulateRoutedMouseEvent(blink::WebInputEvent::kMouseMove, 10, 30, 0, true);
  EXPECT_EQ(blink::WebInputEvent::kTouchMove,
            observer.acked_touch_event_type());
  dispatched_events = observer.GetAndResetDispatchedEventTypes();
  ASSERT_EQ(4u, dispatched_events.size());
  EXPECT_EQ(blink::WebInputEvent::kTouchMove, dispatched_events[0]);
  EXPECT_EQ(blink::WebInputEvent::kGestureTapCancel, dispatched_events[1]);
  EXPECT_EQ(blink::WebInputEvent::kGestureScrollBegin, dispatched_events[2]);
  EXPECT_EQ(blink::WebInputEvent::kGestureScrollUpdate, dispatched_events[3]);
  EXPECT_EQ(0u, observer.GetAndResetDispatchedEventTypes().size());

  // Turn off emulation during a scroll.
  host()->GetTouchEmulator()->Disable();
  EXPECT_EQ(blink::WebInputEvent::kTouchCancel,
            observer.acked_touch_event_type());

  dispatched_events = observer.GetAndResetDispatchedEventTypes();
  ASSERT_EQ(2u, dispatched_events.size());
  EXPECT_EQ(blink::WebInputEvent::kTouchCancel, dispatched_events[0]);
  EXPECT_EQ(blink::WebInputEvent::kGestureScrollEnd, dispatched_events[1]);

  host()->RemoveInputEventObserver(&observer);
}

}  // namespace content
