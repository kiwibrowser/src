// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/auto_reset.h"
#include "base/command_line.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "content/browser/gpu/compositor_util.h"
#include "content/browser/renderer_host/render_widget_host_impl.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/common/input/synthetic_web_input_event_builders.h"
#include "content/common/input_messages.h"
#include "content/public/browser/browser_message_filter.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/shell/browser/shell.h"
#include "third_party/blink/public/platform/web_input_event.h"
#include "ui/latency/latency_info.h"

using blink::WebInputEvent;

namespace {

void GiveItSomeTime() {
  base::RunLoop run_loop;
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, run_loop.QuitClosure(), base::TimeDelta::FromMilliseconds(10));
  run_loop.Run();
}

const char kTouchEventDataURL[] =
    "data:text/html;charset=utf-8,"
#if defined(OS_ANDROID)
    "<head>"
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
    "</head>"
#endif
    "<body onload='setup();'>"
    "<div id='first'></div><div id='second'></div><div id='third'></div>"
    "<style>"
    "  %23first {"
    "    position: absolute;"
    "    width: 100px;"
    "    height: 100px;"
    "    top: 0px;"
    "    left: 0px;"
    "    background-color: green;"
    "    -webkit-transform: translate3d(0, 0, 0);"
    "  }"
    "  %23second {"
    "    position: absolute;"
    "    width: 100px;"
    "    height: 100px;"
    "    top: 0px;"
    "    left: 110px;"
    "    background-color: blue;"
    "    -webkit-transform: translate3d(0, 0, 0);"
    "  }"
    "  %23third {"
    "    position: absolute;"
    "    width: 100px;"
    "    height: 100px;"
    "    top: 110px;"
    "    left: 0px;"
    "    background-color: yellow;"
    "    -webkit-transform: translate3d(0, 0, 0);"
    "  }"
    "</style>"
    "<script>"
    "  function setup() {"
    "    second.ontouchstart = function() {};"
    "    third.ontouchstart = function(e) {"
    "      e.preventDefault();"
    "    };"
    "  }"
    "</script>";

}  // namespace

namespace content {

class TouchInputBrowserTest : public ContentBrowserTest {
 public:
  TouchInputBrowserTest() {}
  ~TouchInputBrowserTest() override {}

  RenderWidgetHostImpl* GetWidgetHost() {
    return RenderWidgetHostImpl::From(
        shell()->web_contents()->GetRenderViewHost()->GetWidget());
  }

  std::unique_ptr<InputMsgWatcher> AddFilter(blink::WebInputEvent::Type type) {
    return std::make_unique<InputMsgWatcher>(GetWidgetHost(), type);
  }

 protected:
  void SendTouchEvent(SyntheticWebTouchEvent* event) {
    GetWidgetHost()->ForwardTouchEventWithLatencyInfo(*event,
                                                      ui::LatencyInfo());
    event->ResetPoints();
  }
  void LoadURL() {
    const GURL data_url(kTouchEventDataURL);
    NavigateToURL(shell(), data_url);

    RenderWidgetHostImpl* host = GetWidgetHost();
    host->GetView()->SetSize(gfx::Size(400, 400));

    // The page is loaded in the renderer, wait for a new frame to arrive.
    while (!host->RequestRepaintForTesting())
      GiveItSomeTime();
  }

  void SetUpCommandLine(base::CommandLine* cmd) override {
    cmd->AppendSwitchASCII(switches::kTouchEventFeatureDetection,
                           switches::kTouchEventFeatureDetectionEnabled);
  }
};

#if defined(OS_MACOSX)
// TODO(ccameron): Failing on mac: crbug.com/346363
#define MAYBE_TouchNoHandler DISABLED_TouchNoHandler
#else
#define MAYBE_TouchNoHandler TouchNoHandler
#endif
IN_PROC_BROWSER_TEST_F(TouchInputBrowserTest, MAYBE_TouchNoHandler) {
  LoadURL();
  SyntheticWebTouchEvent touch;

  // A press on |first| should be acked with NO_CONSUMER_EXISTS since there is
  // no touch-handler on it.
  touch.PressPoint(25, 25);
  auto filter = AddFilter(WebInputEvent::kTouchStart);
  SendTouchEvent(&touch);

  EXPECT_EQ(INPUT_EVENT_ACK_STATE_NO_CONSUMER_EXISTS, filter->WaitForAck());

  // If a touch-press is acked with NO_CONSUMER_EXISTS, then subsequent
  // touch-points don't need to be dispatched until the touch point is released.
  touch.ReleasePoint(0);
  SendTouchEvent(&touch);
}

#if defined(OS_CHROMEOS)
// crbug.com/514456
#define MAYBE_TouchHandlerNoConsume DISABLED_TouchHandlerNoConsume
#else
#define MAYBE_TouchHandlerNoConsume TouchHandlerNoConsume
#endif
IN_PROC_BROWSER_TEST_F(TouchInputBrowserTest, MAYBE_TouchHandlerNoConsume) {
  LoadURL();
  SyntheticWebTouchEvent touch;

  // Press on |second| should be acked with NOT_CONSUMED since there is a
  // touch-handler on |second|, but it doesn't consume the event.
  touch.PressPoint(125, 25);
  auto filter = AddFilter(WebInputEvent::kTouchStart);
  SendTouchEvent(&touch);
  EXPECT_EQ(INPUT_EVENT_ACK_STATE_NOT_CONSUMED, filter->WaitForAck());

  filter = AddFilter(WebInputEvent::kTouchEnd);
  touch.ReleasePoint(0);
  SendTouchEvent(&touch);
  filter->WaitForAck();
}

#if defined(OS_CHROMEOS)
// crbug.com/514456
#define MAYBE_TouchHandlerConsume DISABLED_TouchHandlerConsume
#else
#define MAYBE_TouchHandlerConsume TouchHandlerConsume
#endif
IN_PROC_BROWSER_TEST_F(TouchInputBrowserTest, MAYBE_TouchHandlerConsume) {
  LoadURL();
  SyntheticWebTouchEvent touch;

  // Press on |third| should be acked with CONSUMED since the touch-handler on
  // |third| consimes the event.
  touch.PressPoint(25, 125);
  auto filter = AddFilter(WebInputEvent::kTouchStart);
  SendTouchEvent(&touch);
  EXPECT_EQ(INPUT_EVENT_ACK_STATE_CONSUMED, filter->WaitForAck());

  touch.ReleasePoint(0);
  filter = AddFilter(WebInputEvent::kTouchEnd);
  SendTouchEvent(&touch);
  filter->WaitForAck();
}

#if defined(OS_CHROMEOS)
// crbug.com/514456
#define MAYBE_MultiPointTouchPress DISABLED_MultiPointTouchPress
#elif defined(OS_MACOSX)
// TODO(ccameron): Failing on mac: crbug.com/346363
#define MAYBE_MultiPointTouchPress DISABLED_MultiPointTouchPress
#else
#define MAYBE_MultiPointTouchPress MultiPointTouchPress
#endif
IN_PROC_BROWSER_TEST_F(TouchInputBrowserTest, MAYBE_MultiPointTouchPress) {
  LoadURL();
  SyntheticWebTouchEvent touch;

  // Press on |first|, which sould be acked with NO_CONSUMER_EXISTS. Then press
  // on |third|. That point should be acked with CONSUMED.
  touch.PressPoint(25, 25);
  auto filter = AddFilter(WebInputEvent::kTouchStart);
  SendTouchEvent(&touch);
  EXPECT_EQ(INPUT_EVENT_ACK_STATE_NO_CONSUMER_EXISTS, filter->WaitForAck());

  touch.PressPoint(25, 125);
  filter = AddFilter(WebInputEvent::kTouchStart);
  SendTouchEvent(&touch);
  EXPECT_EQ(INPUT_EVENT_ACK_STATE_CONSUMED, filter->WaitForAck());
}

}  // namespace content
