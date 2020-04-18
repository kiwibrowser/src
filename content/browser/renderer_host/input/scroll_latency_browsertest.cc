// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/bind.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/test/histogram_tester.h"
#include "content/browser/renderer_host/input/synthetic_gesture.h"
#include "content/browser/renderer_host/input/synthetic_gesture_controller.h"
#include "content/browser/renderer_host/input/synthetic_gesture_target.h"
#include "content/browser/renderer_host/input/synthetic_smooth_scroll_gesture.h"
#include "content/browser/renderer_host/render_widget_host_impl.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/common/input/synthetic_gesture_params.h"
#include "content/common/input/synthetic_smooth_scroll_gesture_params.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/shell/browser/shell.h"

namespace {

const char kDataURL[] =
    "data:text/html;charset=utf-8,"
    "<!DOCTYPE html>"
    "<html>"
    "<head>"
    "<title>Mouse wheel latency histograms reported.</title>"
    "<script src=\"../../resources/testharness.js\"></script>"
    "<script src=\"../../resources/testharnessreport.js\"></script>"
    "<style>"
    "body {"
    "  height:3000px;"
    "}"
    "</style>"
    "</head>"
    "<body>"
    "</body>"
    "</html>";

}  // namespace

namespace content {

class ScrollLatencyBrowserTest : public ContentBrowserTest {
 public:
  ScrollLatencyBrowserTest() : loop_(base::MessageLoop::TYPE_UI) {}
  ~ScrollLatencyBrowserTest() override {}

  RenderWidgetHostImpl* GetWidgetHost() {
    return RenderWidgetHostImpl::From(
        shell()->web_contents()->GetRenderViewHost()->GetWidget());
  }

  // TODO(tdresser): Find a way to avoid sleeping like this. See
  // crbug.com/405282 for details.
  void GiveItSomeTime() {
    base::RunLoop run_loop;
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE, run_loop.QuitClosure(),
        base::TimeDelta::FromMillisecondsD(10));
    run_loop.Run();
  }

  void WaitAFrame() {
    while (!GetWidgetHost()->RequestRepaintForTesting())
      GiveItSomeTime();
    frame_observer_->Wait();
  }

 protected:
  void LoadURL() {
    const GURL data_url(kDataURL);
    NavigateToURL(shell(), data_url);

    RenderWidgetHostImpl* host = GetWidgetHost();
    host->GetView()->SetSize(gfx::Size(400, 400));

    frame_observer_ = std::make_unique<MainThreadFrameObserver>(
        shell()->web_contents()->GetRenderViewHost()->GetWidget());

    // Wait a frame to make sure the page has renderered.
    WaitAFrame();
    frame_observer_.reset();
  }

  // Generate a single wheel tick, scrolling by |distance|. This will perform a
  // smooth scroll on platforms which support it.
  void DoSmoothWheelScroll(const gfx::Vector2d& distance) {
    blink::WebGestureEvent event =
        SyntheticWebGestureEventBuilder::BuildScrollBegin(
            distance.x(), -distance.y(),
            blink::WebGestureDevice::kWebGestureDeviceTouchpad, 1);
    event.data.scroll_begin.delta_hint_units =
        blink::WebGestureEvent::ScrollUnits::kPixels;
    GetWidgetHost()->ForwardGestureEvent(event);

    blink::WebGestureEvent event2 =
        SyntheticWebGestureEventBuilder::BuildScrollUpdate(
            distance.x(), -distance.y(), 0,
            blink::WebGestureDevice::kWebGestureDeviceTouchpad);
    event2.data.scroll_update.delta_units =
        blink::WebGestureEvent::ScrollUnits::kPixels;
    GetWidgetHost()->ForwardGestureEvent(event2);
  }

 private:
  base::MessageLoop loop_;
  base::RunLoop runner_;
  std::unique_ptr<MainThreadFrameObserver> frame_observer_;

  DISALLOW_COPY_AND_ASSIGN(ScrollLatencyBrowserTest);
};

// Perform a smooth wheel scroll, and verify that our end-to-end wheel latency
// metric is recorded. See crbug.com/599910 for details.
IN_PROC_BROWSER_TEST_F(ScrollLatencyBrowserTest, SmoothWheelScroll) {
  LoadURL();

  base::HistogramTester histogram_tester;
  DoSmoothWheelScroll(gfx::Vector2d(0, 100));

  size_t num_samples = 0;

  while (num_samples == 0) {
    FetchHistogramsFromChildProcesses();
    num_samples =
        histogram_tester
            .GetAllSamples(
                "Event.Latency.ScrollBegin.Wheel.TimeToScrollUpdateSwapBegin2")
            .size();
    GiveItSomeTime();
  }
}

}  // namespace content
