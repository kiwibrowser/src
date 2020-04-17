// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/run_loop.h"
#include "content/browser/renderer_host/render_widget_host_impl.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/shell/browser/shell.h"

namespace {

const char kTouchpadPinchDataURL[] =
    "data:text/html;charset=utf-8,"
    "<!DOCTYPE html>"
    "<style>"
    "html,body {"
    " height: 100%;"
    "}"
    "</style>"
    "<p>Hello.</p>"
    "<script>"
    "  var resolveHandlerPromise = null;"
    "  var handlerPromise = new Promise(function(resolve) {"
    "    resolveHandlerPromise = resolve;"
    "  });"
    "  function preventPinchListener(e) {"
    "    e.preventDefault();"
    "    resolveHandlerPromise(e);"
    "  }"
    "  function allowPinchListener(e) {"
    "    resolveHandlerPromise(e);"
    "  }"
    "  function setListener(prevent) {"
    "    document.body.addEventListener("
    "        'wheel',"
    "        (prevent ? preventPinchListener : allowPinchListener),"
    "        {passive: false});"
    "  }"
    "  function reset() {"
    "    document.body.removeEventListener("
    "        'wheel', preventPinchListener, {passive: false});"
    "    document.body.removeEventListener("
    "        'wheel', allowPinchListener, {passive: false});"
    "    handlerPromise = new Promise(function(resolve) {"
    "      resolveHandlerPromise = resolve;"
    "    });"
    "  }"
    "</script>";

}  // namespace

namespace content {

class TouchpadPinchBrowserTest : public ContentBrowserTest {
 public:
  TouchpadPinchBrowserTest() = default;
  ~TouchpadPinchBrowserTest() override = default;

 protected:
  void LoadURL() {
    const GURL data_url(kTouchpadPinchDataURL);
    NavigateToURL(shell(), data_url);
    SynchronizeThreads();
  }

  RenderWidgetHostImpl* GetRenderWidgetHost() {
    return RenderWidgetHostImpl::From(shell()
                                          ->web_contents()
                                          ->GetRenderWidgetHostView()
                                          ->GetRenderWidgetHost());
  }

  void SynchronizeThreads() {
    MainThreadFrameObserver observer(GetRenderWidgetHost());
    observer.Wait();
  }
};

namespace {

class TestPageScaleObserver : public WebContentsObserver {
 public:
  explicit TestPageScaleObserver(WebContents* web_contents)
      : WebContentsObserver(web_contents) {}

  void OnPageScaleFactorChanged(float page_scale_factor) override {
    last_scale_ = page_scale_factor;
    seen_page_scale_change_ = true;
    if (done_callback_)
      std::move(done_callback_).Run();
  }

  float WaitForPageScaleUpdate() {
    if (!seen_page_scale_change_) {
      base::RunLoop run_loop;
      done_callback_ = run_loop.QuitClosure();
      run_loop.Run();
    }
    seen_page_scale_change_ = false;
    return last_scale_;
  }

 private:
  base::OnceClosure done_callback_;
  bool seen_page_scale_change_ = false;
  float last_scale_ = 0.f;
};

}  // namespace

// Performing a touchpad pinch gesture should change the page scale.
IN_PROC_BROWSER_TEST_F(TouchpadPinchBrowserTest,
                       TouchpadPinchChangesPageScale) {
  LoadURL();

  TestPageScaleObserver scale_observer(shell()->web_contents());

  const gfx::Rect contents_rect = shell()->web_contents()->GetContainerBounds();
  const gfx::Point pinch_position(contents_rect.width() / 2,
                                  contents_rect.height() / 2);
  SimulateGesturePinchSequence(shell()->web_contents(), pinch_position, 1.23,
                               blink::kWebGestureDeviceTouchpad);

  scale_observer.WaitForPageScaleUpdate();
}

// We should offer synthetic wheel events to the page when a touchpad pinch
// is performed.
IN_PROC_BROWSER_TEST_F(TouchpadPinchBrowserTest, WheelListenerAllowingPinch) {
  LoadURL();
  ASSERT_TRUE(
      content::ExecuteScript(shell()->web_contents(), "setListener(false);"));
  SynchronizeThreads();

  TestPageScaleObserver scale_observer(shell()->web_contents());

  const gfx::Rect contents_rect = shell()->web_contents()->GetContainerBounds();
  const gfx::Point pinch_position(contents_rect.width() / 2,
                                  contents_rect.height() / 2);
  SimulateGesturePinchSequence(shell()->web_contents(), pinch_position, 1.23,
                               blink::kWebGestureDeviceTouchpad);

  // Ensure that the page saw the synthetic wheel.
  bool default_prevented = false;
  ASSERT_TRUE(content::ExecuteScriptAndExtractBool(
      shell()->web_contents(),
      "handlerPromise.then(function(e) {"
      "  window.domAutomationController.send(e.defaultPrevented);"
      "});",
      &default_prevented));
  EXPECT_FALSE(default_prevented);

  // Since the listener did not cancel the synthetic wheel, we should still
  // change the page scale.
  scale_observer.WaitForPageScaleUpdate();
}

// If the synthetic wheel event for a touchpad pinch is canceled, we should not
// change the page scale.
IN_PROC_BROWSER_TEST_F(TouchpadPinchBrowserTest, WheelListenerPreventingPinch) {
  LoadURL();

  // Perform an initial pinch so we can figure out the page scale we're
  // starting with for the test proper.
  TestPageScaleObserver starting_scale_observer(shell()->web_contents());
  const gfx::Rect contents_rect = shell()->web_contents()->GetContainerBounds();
  const gfx::Point pinch_position(contents_rect.width() / 2,
                                  contents_rect.height() / 2);
  SimulateGesturePinchSequence(shell()->web_contents(), pinch_position, 1.23,
                               blink::kWebGestureDeviceTouchpad);
  const float starting_scale_factor =
      starting_scale_observer.WaitForPageScaleUpdate();
  ASSERT_GT(starting_scale_factor, 0.f);

  ASSERT_TRUE(
      content::ExecuteScript(shell()->web_contents(), "setListener(true);"));
  SynchronizeThreads();

  SimulateGesturePinchSequence(shell()->web_contents(), pinch_position, 1.5,
                               blink::kWebGestureDeviceTouchpad);

  // Ensure the page handled a wheel event that it was able to cancel.
  bool default_prevented = false;
  ASSERT_TRUE(content::ExecuteScriptAndExtractBool(
      shell()->web_contents(),
      "handlerPromise.then(function(e) {"
      "  window.domAutomationController.send(e.defaultPrevented);"
      "});",
      &default_prevented));
  EXPECT_TRUE(default_prevented);

  // We'll check that the previous pinch did not cause a scale change by
  // performing another pinch that does change the scale.
  ASSERT_TRUE(content::ExecuteScript(shell()->web_contents(),
                                     "reset(); "
                                     "setListener(false);"));
  SynchronizeThreads();

  TestPageScaleObserver scale_observer(shell()->web_contents());
  SimulateGesturePinchSequence(shell()->web_contents(), pinch_position, 2.0,
                               blink::kWebGestureDeviceTouchpad);
  ASSERT_TRUE(content::ExecuteScriptAndExtractBool(
      shell()->web_contents(),
      "handlerPromise.then(function(e) {"
      "  window.domAutomationController.send(e.defaultPrevented);"
      "});",
      &default_prevented));
  EXPECT_FALSE(default_prevented);

  const float last_scale_factor = scale_observer.WaitForPageScaleUpdate();
  EXPECT_FLOAT_EQ(starting_scale_factor * 2.0, last_scale_factor);
}

}  // namespace content
