// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/html/html_frame_owner_element.h"

#include <algorithm>

#include "third_party/blink/renderer/core/exported/web_view_impl.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/testing/sim/sim_compositor.h"
#include "third_party/blink/renderer/core/testing/sim/sim_request.h"
#include "third_party/blink/renderer/core/testing/sim/sim_test.h"
#include "third_party/blink/renderer/platform/geometry/float_rect.h"
#include "third_party/blink/renderer/platform/testing/runtime_enabled_features_test_helpers.h"
#include "third_party/blink/renderer/platform/testing/unit_test_helpers.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

namespace {

class LazyLoadFramesTest : public SimTest {
 public:
  static constexpr int kViewportWidth = 800;
  static constexpr int kViewportHeight = 600;

  LazyLoadFramesTest() : scoped_lazy_frame_loading_for_test_(true) {}

  void SetUp() override {
    SimTest::SetUp();
    WebView().Resize(WebSize(kViewportWidth, kViewportHeight));
  }

 private:
  ScopedLazyFrameLoadingForTest scoped_lazy_frame_loading_for_test_;
};

TEST_F(LazyLoadFramesTest, SameOriginFrameIsNotDeferred) {
  SimRequest main_resource("https://example.com/", "text/html");
  SimRequest child_frame_resource("https://example.com/subframe.html",
                                  "text/html");

  LoadURL("https://example.com/");

  main_resource.Complete(String::Format(
      R"HTML(
        <body onload='console.log("main body onload");'>
        <div style='height: %dpx;'></div>
        <iframe src='https://example.com/subframe.html'
             style='width: 200px; height: 200px;'
             onload='console.log("child frame element onload");'></iframe>
        </body>)HTML",
      kViewportHeight + HTMLFrameOwnerElement::kLazyLoadRootMarginPx));

  child_frame_resource.Complete("");

  Compositor().BeginFrame();
  test::RunPendingTasks();

  EXPECT_TRUE(ConsoleMessages().Contains("main body onload"));
  EXPECT_TRUE(ConsoleMessages().Contains("child frame element onload"));
}

TEST_F(LazyLoadFramesTest, AboveTheFoldFrameIsNotDeferred) {
  SimRequest main_resource("https://example.com/", "text/html");
  SimRequest child_frame_resource("https://crossorigin.com/subframe.html",
                                  "text/html");

  LoadURL("https://example.com/");

  main_resource.Complete(String::Format(
      R"HTML(
        <body onload='console.log("main body onload");'>
        <div style='height: %dpx;'></div>
        <iframe src='https://crossorigin.com/subframe.html'
             style='width: 200px; height: 200px;'
             onload='console.log("child frame element onload");'></iframe>
        </body>)HTML",
      kViewportHeight - 100));

  Compositor().BeginFrame();
  test::RunPendingTasks();

  child_frame_resource.Complete("");

  test::RunPendingTasks();

  EXPECT_TRUE(ConsoleMessages().Contains("main body onload"));
  EXPECT_TRUE(ConsoleMessages().Contains("child frame element onload"));
}

TEST_F(LazyLoadFramesTest, BelowTheFoldButNearViewportFrameIsNotDeferred) {
  SimRequest main_resource("https://example.com/", "text/html");
  SimRequest child_frame_resource("https://crossorigin.com/subframe.html",
                                  "text/html");

  LoadURL("https://example.com/");

  main_resource.Complete(String::Format(
      R"HTML(
        <body onload='console.log("main body onload");'>
        <div style='height: %dpx;'></div>
        <iframe src='https://crossorigin.com/subframe.html'
             style='width: 200px; height: 200px;'
             onload='console.log("child frame element onload");'></iframe>
        </body>)HTML",
      kViewportHeight + 100));

  Compositor().BeginFrame();
  test::RunPendingTasks();

  child_frame_resource.Complete("");

  test::RunPendingTasks();

  EXPECT_TRUE(ConsoleMessages().Contains("main body onload"));
  EXPECT_TRUE(ConsoleMessages().Contains("child frame element onload"));
}

TEST_F(LazyLoadFramesTest, HiddenAndTinyFramesAreNotDeferred) {
  SimRequest main_resource("https://example.com/", "text/html");

  SimRequest display_none_frame_resource(
      "https://crossorigin.com/display_none.html", "text/html");
  SimRequest tiny_frame_resource("https://crossorigin.com/tiny.html",
                                 "text/html");
  SimRequest tiny_width_frame_resource(
      "https://crossorigin.com/tiny_width.html", "text/html");
  SimRequest tiny_height_frame_resource(
      "https://crossorigin.com/tiny_height.html", "text/html");
  SimRequest off_screen_left_frame_resource(
      "https://crossorigin.com/off_screen_left.html", "text/html");
  SimRequest off_screen_top_frame_resource(
      "https://crossorigin.com/off_screen_top.html", "text/html");

  LoadURL("https://example.com/");

  main_resource.Complete(String::Format(
      R"HTML(
        <head><style>
          /* Chrome by default sets borders for iframes, so explicitly specify
           * no borders, padding, or margins here so that the dimensions of the
           * tiny frames aren't artifically inflated past the dimensions that
           * the lazy loading logic considers "tiny". */
          iframe { border-style: none; padding: 0px; margin: 0px; }
        </style></head>

        <body onload='console.log("main body onload");'>
        <div style='height: %dpx'></div>
        <iframe src='https://crossorigin.com/display_none.html'
             style='display: none;'
             onload='console.log("display none element onload");'></iframe>
        <iframe src='https://crossorigin.com/tiny.html'
             style='width: 4px; height: 4px;'
             onload='console.log("tiny element onload");'></iframe>
        <iframe src='https://crossorigin.com/tiny_width.html'
             style='width: 0px; height: 200px;'
             onload='console.log("tiny width element onload");'></iframe>
        <iframe src='https://crossorigin.com/tiny_height.html'
             style='width: 200px; height: 0px;'
             onload='console.log("tiny height element onload");'></iframe>
        <iframe src='https://crossorigin.com/off_screen_left.html'
             style='position:relative;right:9000px;width:200px;height:200px;'
             onload='console.log("off screen left element onload");'></iframe>
        <iframe src='https://crossorigin.com/off_screen_top.html'
             style='position:relative;bottom:9000px;width:200px;height:200px;'
             onload='console.log("off screen top element onload");'></iframe>
        </body>
      )HTML",
      kViewportHeight + HTMLFrameOwnerElement::kLazyLoadRootMarginPx + 100));

  Compositor().BeginFrame();
  test::RunPendingTasks();

  display_none_frame_resource.Complete("");
  tiny_frame_resource.Complete("");
  tiny_width_frame_resource.Complete("");
  tiny_height_frame_resource.Complete("");
  off_screen_left_frame_resource.Complete("");
  off_screen_top_frame_resource.Complete("");

  test::RunPendingTasks();

  EXPECT_TRUE(ConsoleMessages().Contains("main body onload"));
  EXPECT_TRUE(ConsoleMessages().Contains("display none element onload"));
  EXPECT_TRUE(ConsoleMessages().Contains("tiny element onload"));
  EXPECT_TRUE(ConsoleMessages().Contains("tiny width element onload"));
  EXPECT_TRUE(ConsoleMessages().Contains("tiny height element onload"));
  EXPECT_TRUE(ConsoleMessages().Contains("off screen left element onload"));
  EXPECT_TRUE(ConsoleMessages().Contains("off screen top element onload"));
}

TEST_F(LazyLoadFramesTest, CrossOriginFrameIsDeferredUntilNearViewport) {
  SimRequest main_resource("https://example.com/", "text/html");
  LoadURL("https://example.com/");

  main_resource.Complete(String::Format(
      R"HTML(
        <body onload='console.log("main body onload");'>
        <div style='height: %dpx;'></div>
        <iframe src='https://crossorigin.com/subframe.html'
             style='width: 400px; height: 400px;'
             onload='console.log("child frame element onload");'></iframe>
        </body>)HTML",
      kViewportHeight + HTMLFrameOwnerElement::kLazyLoadRootMarginPx + 100));

  Compositor().BeginFrame();
  test::RunPendingTasks();

  EXPECT_TRUE(ConsoleMessages().Contains("main body onload"));
  EXPECT_FALSE(ConsoleMessages().Contains("child frame element onload"));

  SimRequest child_frame_resource("https://crossorigin.com/subframe.html",
                                  "text/html");

  // Scroll down near the child frame. This should cause the child frame to get
  // loaded.
  GetDocument().View()->LayoutViewportScrollableArea()->SetScrollOffset(
      ScrollOffset(0, kViewportHeight + 150), kProgrammaticScroll);

  Compositor().BeginFrame();
  test::RunPendingTasks();

  SimRequest nested_child_frame_resource("https://test.com/", "text/html");

  // There's another nested cross origin iframe inside the first child frame,
  // partway down such that it's not near the viewport. It should still be
  // loaded immediately, and not deferred, since it's nested inside a frame that
  // was previously deferred.
  child_frame_resource.Complete(
      "<div style='height: 200px;'></div>"
      "<iframe src='https://test.com/' style='width: 200px; height: 200px;'>"
      "</iframe>");

  test::RunPendingTasks();
  nested_child_frame_resource.Complete("");
  test::RunPendingTasks();

  EXPECT_TRUE(ConsoleMessages().Contains("main body onload"));
  EXPECT_TRUE(ConsoleMessages().Contains("child frame element onload"));
}

TEST_F(LazyLoadFramesTest, AboutBlankNavigationIsNotDeferred) {
  SimRequest main_resource("https://example.com/", "text/html");
  SimRequest child_frame_resource("https://crossorigin.com/subframe.html",
                                  "text/html");

  LoadURL("https://example.com/");

  main_resource.Complete(String::Format(
      R"HTML(
        <body onload='BodyOnload()'>
        <script>
          function BodyOnload() {
            console.log('main body onload');
            document.getElementsByTagName('iframe')[0].src =
                'https://crossorigin.com/subframe.html';
          }
        </script>

        <div style='height: %dpx;'></div>
        <iframe
             style='width: 200px; height: 200px;'
             onload='console.log("child frame element onload");'></iframe>
        </body>)HTML",
      kViewportHeight + HTMLFrameOwnerElement::kLazyLoadRootMarginPx + 100));

  Compositor().BeginFrame();
  test::RunPendingTasks();

  EXPECT_TRUE(ConsoleMessages().Contains("main body onload"));
  EXPECT_EQ(1, static_cast<int>(std::count(ConsoleMessages().begin(),
                                           ConsoleMessages().end(),
                                           "child frame element onload")));

  child_frame_resource.Complete("");

  Compositor().BeginFrame();
  test::RunPendingTasks();

  EXPECT_EQ(2, static_cast<int>(std::count(ConsoleMessages().begin(),
                                           ConsoleMessages().end(),
                                           "child frame element onload")));
}

TEST_F(LazyLoadFramesTest, JavascriptStringUrlIsNotDeferred) {
  SimRequest main_resource("https://example.com/", "text/html");
  LoadURL("https://example.com/");

  main_resource.Complete(String::Format(
      R"HTML(
        <body onload='console.log("main body onload");'>
        <div style='height: %dpx;'></div>
        <iframe src='javascript:"Hello World!";'
             style='width: 200px; height: 200px;'
             onload='console.log("child frame element onload");'></iframe>
        </body>)HTML",
      kViewportHeight + HTMLFrameOwnerElement::kLazyLoadRootMarginPx + 100));

  EXPECT_TRUE(ConsoleMessages().Contains("main body onload"));
  EXPECT_TRUE(ConsoleMessages().Contains("child frame element onload"));
}

}  // namespace

}  // namespace blink
