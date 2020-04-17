// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "build/build_config.h"
#include "content/browser/renderer_host/input/synthetic_smooth_scroll_gesture.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/shell/browser/shell.h"
#include "third_party/blink/public/platform/web_input_event.h"
#include "ui/events/base_event_utils.h"

using blink::WebInputEvent;

namespace {

const std::string kBrowserFlingDataURL = R"HTML(
  <!DOCTYPE html>
  <meta name='viewport' content='width=device-width'/>
  <style>
  html, body {
    margin: 0;
  }
  .spacer { height: 10000px; }
  </style>
  <div class=spacer></div>
  <script>
    document.title='ready';
  </script>)HTML";

const std::string kTouchActionFilterDataURL = R"HTML(
  <!DOCTYPE html>
  <meta name='viewport' content='width=device-width'/>
  <style>
    body {
      height: 10000px;
      touch-action: pan-y;
    }
  </style>
  <script>
    document.title='ready';
  </script>)HTML";
}  // namespace

namespace content {

class BrowserSideFlingBrowserTest : public ContentBrowserTest {
 public:
  BrowserSideFlingBrowserTest() {}
  ~BrowserSideFlingBrowserTest() override {}

  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitchASCII("--enable-blink-features",
                                    "MiddleClickAutoscroll");
  }

  void OnSyntheticGestureCompleted(SyntheticGesture::Result result) {
    EXPECT_EQ(SyntheticGesture::GESTURE_FINISHED, result);
    run_loop_->Quit();
  }

 protected:
  RenderWidgetHostImpl* GetWidgetHost() {
    return RenderWidgetHostImpl::From(
        shell()->web_contents()->GetRenderViewHost()->GetWidget());
  }

  void LoadURL(const std::string& page_data) {
    const GURL data_url("data:text/html," + page_data);
    NavigateToURL(shell(), data_url);

    RenderWidgetHostImpl* host = GetWidgetHost();
    host->GetView()->SetSize(gfx::Size(400, 400));

    base::string16 ready_title(base::ASCIIToUTF16("ready"));
    TitleWatcher watcher(shell()->web_contents(), ready_title);
    ignore_result(watcher.WaitAndGetTitle());

    MainThreadFrameObserver main_thread_sync(host);
    main_thread_sync.Wait();
  }

  void SimulateMiddleClick(int x, int y, int modifiers) {
    // Simulate and send middle click mouse down.
    blink::WebMouseEvent down_event = SyntheticWebMouseEventBuilder::Build(
        blink::WebInputEvent::kMouseDown, x, y, modifiers);
    down_event.button = blink::WebMouseEvent::Button::kMiddle;
    down_event.SetTimeStamp(ui::EventTimeForNow());
    down_event.SetPositionInScreen(x, y);
    GetWidgetHost()->ForwardMouseEvent(down_event);

    // Simulate and send middle click mouse up.
    blink::WebMouseEvent up_event = SyntheticWebMouseEventBuilder::Build(
        blink::WebInputEvent::kMouseUp, x, y, modifiers);
    up_event.button = blink::WebMouseEvent::Button::kMiddle;
    up_event.SetTimeStamp(ui::EventTimeForNow());
    up_event.SetPositionInScreen(x, y);
    GetWidgetHost()->ForwardMouseEvent(up_event);
  }

  std::unique_ptr<base::RunLoop> run_loop_;

 private:
  DISALLOW_COPY_AND_ASSIGN(BrowserSideFlingBrowserTest);
};

IN_PROC_BROWSER_TEST_F(BrowserSideFlingBrowserTest, TouchscreenFling) {
  LoadURL(kBrowserFlingDataURL);

  // Send a GSB to start scrolling sequence.
  blink::WebGestureEvent gesture_scroll_begin(
      blink::WebGestureEvent::kGestureScrollBegin,
      blink::WebInputEvent::kNoModifiers, ui::EventTimeForNow());
  gesture_scroll_begin.SetSourceDevice(blink::kWebGestureDeviceTouchscreen);
  gesture_scroll_begin.data.scroll_begin.delta_hint_units =
      blink::WebGestureEvent::ScrollUnits::kPrecisePixels;
  gesture_scroll_begin.data.scroll_begin.delta_x_hint = 0.f;
  gesture_scroll_begin.data.scroll_begin.delta_y_hint = -5.f;
  GetWidgetHost()->ForwardGestureEvent(gesture_scroll_begin);

  //  Send a GFS and wait for the page to scroll making sure that fling progress
  //  has started.
  blink::WebGestureEvent gesture_fling_start(
      blink::WebGestureEvent::kGestureFlingStart,
      blink::WebInputEvent::kNoModifiers, ui::EventTimeForNow());
  gesture_fling_start.SetSourceDevice(blink::kWebGestureDeviceTouchscreen);
  gesture_fling_start.data.fling_start.velocity_x = 0.f;
  gesture_fling_start.data.fling_start.velocity_y = -2000.f;
  GetWidgetHost()->ForwardGestureEvent(gesture_fling_start);
  RenderFrameSubmissionObserver observer(
      GetWidgetHost()->render_frame_metadata_provider());
  gfx::Vector2dF default_scroll_offset;
  while (observer.LastRenderFrameMetadata()
             .root_scroll_offset.value_or(default_scroll_offset)
             .y() <= 0) {
    observer.WaitForMetadataChange();
  }
}

IN_PROC_BROWSER_TEST_F(BrowserSideFlingBrowserTest, TouchpadFling) {
  LoadURL(kBrowserFlingDataURL);

  // Send a wheel event to start scrolling sequence.
  auto input_msg_watcher = std::make_unique<InputMsgWatcher>(
      GetWidgetHost(), blink::WebInputEvent::kMouseWheel);
  blink::WebMouseWheelEvent wheel_event =
      SyntheticWebMouseWheelEventBuilder::Build(10, 10, 0, -53, 0, true);
  wheel_event.phase = blink::WebMouseWheelEvent::kPhaseBegan;
  GetWidgetHost()->ForwardWheelEvent(wheel_event);
  input_msg_watcher->WaitForAck();

  //  Send a GFS and wait for the page to scroll more than 60 pixels making sure
  //  that fling progress has started.
  blink::WebGestureEvent gesture_fling_start(
      blink::WebGestureEvent::kGestureFlingStart,
      blink::WebInputEvent::kNoModifiers, ui::EventTimeForNow());
  gesture_fling_start.SetSourceDevice(blink::kWebGestureDeviceTouchpad);
  gesture_fling_start.data.fling_start.velocity_x = 0.f;
  gesture_fling_start.data.fling_start.velocity_y = -2000.f;
  GetWidgetHost()->ForwardGestureEvent(gesture_fling_start);
  RenderFrameSubmissionObserver observer(
      GetWidgetHost()->render_frame_metadata_provider());
  gfx::Vector2dF default_scroll_offset;
  while (observer.LastRenderFrameMetadata()
             .root_scroll_offset.value_or(default_scroll_offset)
             .y() <= 60) {
    observer.WaitForMetadataChange();
  }
}

// TODO(sahel): This test is flaking on OS_CHROMEOS https://crbug.com/838769
#if defined(OS_ANDROID) || defined(OS_CHROMEOS)
#define MAYBE_AutoscrollFling DISABLED_AutoscrollFling
#else
#define MAYBE_AutoscrollFling AutoscrollFling
#endif
IN_PROC_BROWSER_TEST_F(BrowserSideFlingBrowserTest, MAYBE_AutoscrollFling) {
  LoadURL(kBrowserFlingDataURL);

  // Start autoscroll with middle click.
  auto input_msg_watcher = std::make_unique<InputMsgWatcher>(
      GetWidgetHost(), blink::WebInputEvent::kGestureScrollBegin);
  SimulateMiddleClick(10, 10, blink::WebInputEvent::kNoModifiers);
  input_msg_watcher->WaitForAck();

  // The page should start scrolling with mouse move.
  RenderFrameSubmissionObserver observer(
      GetWidgetHost()->render_frame_metadata_provider());
  blink::WebMouseEvent move_event = SyntheticWebMouseEventBuilder::Build(
      blink::WebInputEvent::kMouseMove, 30, 30,
      blink::WebInputEvent::kNoModifiers);
  move_event.SetTimeStamp(ui::EventTimeForNow());
  move_event.SetPositionInScreen(30, 30);
  GetWidgetHost()->ForwardMouseEvent(move_event);
  gfx::Vector2dF default_scroll_offset;
  while (observer.LastRenderFrameMetadata()
             .root_scroll_offset.value_or(default_scroll_offset)
             .y() <= 0) {
    observer.WaitForMetadataChange();
  }
}

#if !defined(OS_ANDROID)
#define MAYBE_WheelScrollingWorksAfterAutoscrollCancel \
  WheelScrollingWorksAfterAutoscrollCancel
#else
#define MAYBE_WheelScrollingWorksAfterAutoscrollCancel \
  DISABLED_WheelScrollingWorksAfterAutoscrollCancel
#endif
// Checks that wheel scrolling works after autoscroll cancelation.
IN_PROC_BROWSER_TEST_F(BrowserSideFlingBrowserTest,
                       MAYBE_WheelScrollingWorksAfterAutoscrollCancel) {
  LoadURL(kBrowserFlingDataURL);

  // Start autoscroll with middle click.
  auto input_msg_watcher = std::make_unique<InputMsgWatcher>(
      GetWidgetHost(), blink::WebInputEvent::kGestureScrollBegin);
  SimulateMiddleClick(10, 10, blink::WebInputEvent::kNoModifiers);
  input_msg_watcher->WaitForAck();

  // Without moving the mouse cancel the autoscroll fling with another click.
  input_msg_watcher = std::make_unique<InputMsgWatcher>(
      GetWidgetHost(), blink::WebInputEvent::kGestureScrollEnd);
  SimulateMiddleClick(10, 10, blink::WebInputEvent::kNoModifiers);
  input_msg_watcher->WaitForAck();

  // The mouse wheel scrolling must work after autoscroll cancellation.
  RenderFrameSubmissionObserver observer(
      GetWidgetHost()->render_frame_metadata_provider());
  blink::WebMouseWheelEvent wheel_event =
      SyntheticWebMouseWheelEventBuilder::Build(10, 10, 0, -53, 0, true);
  wheel_event.phase = blink::WebMouseWheelEvent::kPhaseBegan;
  GetWidgetHost()->ForwardWheelEvent(wheel_event);
  gfx::Vector2dF default_scroll_offset;
  while (observer.LastRenderFrameMetadata()
             .root_scroll_offset.value_or(default_scroll_offset)
             .y() <= 0) {
    observer.WaitForMetadataChange();
  }
}

// Disabled on MacOS because it doesn't support touchscreen scroll.
#if defined(OS_MACOSX)
#define MAYBE_ScrollEndGeneratedForFilteredFling \
  DISABLED_ScrollEndGeneratedForFilteredFling
#else
#define MAYBE_ScrollEndGeneratedForFilteredFling \
  ScrollEndGeneratedForFilteredFling
#endif
IN_PROC_BROWSER_TEST_F(BrowserSideFlingBrowserTest,
                       MAYBE_ScrollEndGeneratedForFilteredFling) {
  LoadURL(kTouchActionFilterDataURL);

  // Necessary for checking the ACK source of the sent events. The events are
  // filtered when the Browser is the source.
  auto scroll_begin_watcher = std::make_unique<InputMsgWatcher>(
      GetWidgetHost(), blink::WebInputEvent::kGestureScrollBegin);
  auto fling_start_watcher = std::make_unique<InputMsgWatcher>(
      GetWidgetHost(), blink::WebInputEvent::kGestureFlingStart);
  auto scroll_end_watcher = std::make_unique<InputMsgWatcher>(
      GetWidgetHost(), blink::WebInputEvent::kGestureScrollEnd);

  // Do a horizontal touchscreen scroll followed by a fling. The GFS must get
  // filtered since the GSB is filtered.
  SyntheticSmoothScrollGestureParams params;
  params.gesture_source_type = SyntheticGestureParams::TOUCH_INPUT;
  params.anchor = gfx::PointF(10, 10);
  params.distances.push_back(gfx::Vector2d(-60, 0));
  params.prevent_fling = false;

  run_loop_ = std::make_unique<base::RunLoop>();

  std::unique_ptr<SyntheticSmoothScrollGesture> gesture(
      new SyntheticSmoothScrollGesture(params));
  GetWidgetHost()->QueueSyntheticGesture(
      std::move(gesture),
      base::BindOnce(&BrowserSideFlingBrowserTest::OnSyntheticGestureCompleted,
                     base::Unretained(this)));

  // Runs until we get the OnSyntheticGestureCompleted callback.
  run_loop_->Run();

  scroll_begin_watcher->GetAckStateWaitIfNecessary();
  EXPECT_EQ(InputEventAckSource::BROWSER,
            scroll_begin_watcher->last_event_ack_source());

  fling_start_watcher->GetAckStateWaitIfNecessary();
  EXPECT_EQ(InputEventAckSource::BROWSER,
            fling_start_watcher->last_event_ack_source());

  // Since the GFS is filtered. the input_router_impl will generate and forward
  // a GSE to make sure that the scrolling sequence and the touch action filter
  // state get reset properly. The generated GSE will also get filtered since
  // its equivalent GSB is filtered. The test will timeout if the GSE is not
  // generated.
  scroll_end_watcher->GetAckStateWaitIfNecessary();
  EXPECT_EQ(InputEventAckSource::BROWSER,
            scroll_end_watcher->last_event_ack_source());
}

}  // namespace content
