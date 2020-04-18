// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/core/exported/web_view_impl.h"
#include "third_party/blink/renderer/core/frame/web_local_frame_impl.h"
#include "third_party/blink/renderer/core/testing/sim/sim_compositor.h"
#include "third_party/blink/renderer/core/testing/sim/sim_request.h"
#include "third_party/blink/renderer/core/testing/sim/sim_test.h"
#include "third_party/blink/renderer/platform/testing/histogram_tester.h"
#include "third_party/blink/renderer/platform/testing/unit_test_helpers.h"

namespace blink {

static const char kHistogramName[] =
    "Navigation.DeferredDocumentLoading.StatesV4";

class DeferredLoadingTest : public SimTest {
 protected:
  DeferredLoadingTest() = default;

  void SetUp() override {
    SimTest::SetUp();
    WebView().Resize(WebSize(640, 480));
  }
  void CompositeFrame() {
    while (Compositor().NeedsBeginFrame()) {
      Compositor().BeginFrame();
      test::RunPendingTasks();
    }
  }

  std::unique_ptr<SimRequest> CreateMainResource() {
    std::unique_ptr<SimRequest> main_resource =
        std::make_unique<SimRequest>("https://example.com/", "text/html");
    LoadURL("https://example.com/");
    return main_resource;
  }

  void ExpectCount(WouldLoadReason reason, int count) {
    histogram_tester_.ExpectBucketCount(kHistogramName,
                                        static_cast<int>(reason), count);
  }

  void ExpectTotalCount(int count) {
    histogram_tester_.ExpectTotalCount(kHistogramName, count);
  }

 private:
  HistogramTester histogram_tester_;
};

TEST_F(DeferredLoadingTest, Visible) {
  std::unique_ptr<SimRequest> main_resource = CreateMainResource();

  main_resource->Complete("<iframe sandbox></iframe>");

  CompositeFrame();

  ExpectCount(WouldLoadReason::kCreated, 1);
  ExpectCount(WouldLoadReason::kVisible, 1);
  ExpectCount(WouldLoadReason::k1ScreenAway, 1);
  ExpectCount(WouldLoadReason::k2ScreensAway, 1);
  ExpectCount(WouldLoadReason::k3ScreensAway, 1);
  ExpectTotalCount(5);
}

TEST_F(DeferredLoadingTest, Right) {
  std::unique_ptr<SimRequest> main_resource = CreateMainResource();

  main_resource->Complete(
      "<iframe style='position:absolute; left:105vw;' sandbox></iframe>");

  CompositeFrame();

  ExpectCount(WouldLoadReason::kCreated, 1);
  ExpectTotalCount(1);
}

TEST_F(DeferredLoadingTest, TwoScreensBelow) {
  std::unique_ptr<SimRequest> main_resource = CreateMainResource();

  main_resource->Complete(
      "<iframe style='position:absolute; top:205vh;' sandbox></iframe>");

  CompositeFrame();

  ExpectCount(WouldLoadReason::kCreated, 1);
  ExpectCount(WouldLoadReason::k2ScreensAway, 1);
  ExpectCount(WouldLoadReason::k3ScreensAway, 1);
  ExpectTotalCount(3);
}

TEST_F(DeferredLoadingTest, Above) {
  std::unique_ptr<SimRequest> main_resource = CreateMainResource();

  main_resource->Complete(
      "<iframe style='position:absolute; top:-10000px;' sandbox></iframe>");

  CompositeFrame();

  ExpectCount(WouldLoadReason::kCreated, 1);
  ExpectCount(WouldLoadReason::kVisible, 1);
  ExpectCount(WouldLoadReason::k1ScreenAway, 1);
  ExpectCount(WouldLoadReason::k2ScreensAway, 1);
  ExpectCount(WouldLoadReason::k3ScreensAway, 1);
  ExpectTotalCount(5);
}

TEST_F(DeferredLoadingTest, Left) {
  std::unique_ptr<SimRequest> main_resource = CreateMainResource();

  main_resource->Complete(
      "<iframe style='position:absolute; left:-10000px;' sandbox></iframe>");

  CompositeFrame();

  ExpectCount(WouldLoadReason::kCreated, 1);
  ExpectCount(WouldLoadReason::kVisible, 1);
  ExpectCount(WouldLoadReason::k1ScreenAway, 1);
  ExpectCount(WouldLoadReason::k2ScreensAway, 1);
  ExpectCount(WouldLoadReason::k3ScreensAway, 1);
  ExpectTotalCount(5);
}

TEST_F(DeferredLoadingTest, AboveAndLeft) {
  std::unique_ptr<SimRequest> main_resource = CreateMainResource();

  main_resource->Complete(
      "<iframe style='position:absolute; left:-10000px; top:-10000px' sandbox>"
      "</iframe>");

  CompositeFrame();

  ExpectCount(WouldLoadReason::kCreated, 1);
  ExpectCount(WouldLoadReason::kVisible, 1);
  ExpectTotalCount(5);
}

TEST_F(DeferredLoadingTest, ZeroByZero) {
  std::unique_ptr<SimRequest> main_resource = CreateMainResource();

  main_resource->Complete(
      "<iframe style='height:0px;width:0px;' sandbox></iframe>");

  CompositeFrame();

  ExpectCount(WouldLoadReason::kCreated, 1);
  ExpectCount(WouldLoadReason::kVisible, 1);
}

TEST_F(DeferredLoadingTest, DisplayNone) {
  std::unique_ptr<SimRequest> main_resource = CreateMainResource();

  main_resource->Complete("<iframe style='display:none' sandbox></iframe>");

  CompositeFrame();

  ExpectCount(WouldLoadReason::kCreated, 1);
  ExpectCount(WouldLoadReason::kNoParent, 1);
  ExpectTotalCount(6);
}

TEST_F(DeferredLoadingTest, DisplayNoneIn2ScreensBelow) {
  std::unique_ptr<SimRequest> main_resource = CreateMainResource();
  SimRequest frame_resource("https://example.com/iframe.html", "text/html");

  main_resource->Complete(
      "<iframe style='position:absolute; top:205vh' "
      "src='iframe.html' sandbox></iframe>");
  frame_resource.Complete("<iframe style='display:none' sandbox></iframe>");

  CompositeFrame();

  ExpectCount(WouldLoadReason::kCreated, 2);
  ExpectCount(WouldLoadReason::kNoParent, 1);
  ExpectCount(WouldLoadReason::kVisible, 1);
  ExpectCount(WouldLoadReason::k1ScreenAway, 1);
  ExpectCount(WouldLoadReason::k2ScreensAway, 2);
  ExpectCount(WouldLoadReason::k3ScreensAway, 2);
  ExpectTotalCount(9);
}

TEST_F(DeferredLoadingTest, LeftNestedInBelow) {
  std::unique_ptr<SimRequest> main_resource = CreateMainResource();
  SimRequest frame_resource("https://example.com/iframe.html", "text/html");

  main_resource->Complete(
      "<iframe style='position:absolute; top:105vh;' src='iframe.html' "
      "sandbox></iframe>");
  frame_resource.Complete(
      "<iframe style='position:absolute; left:-10000px;' sandbox></iframe>");

  CompositeFrame();

  ExpectCount(WouldLoadReason::kCreated, 2);
  ExpectCount(WouldLoadReason::k1ScreenAway, 2);
  ExpectCount(WouldLoadReason::k2ScreensAway, 2);
  ExpectCount(WouldLoadReason::k3ScreensAway, 2);
  ExpectTotalCount(8);
}

TEST_F(DeferredLoadingTest, OneScreenBelowThenScriptedVisible) {
  std::unique_ptr<SimRequest> main_resource = CreateMainResource();

  main_resource->Start();
  main_resource->Write(
      "<iframe id='theFrame' style='position:absolute; top:105vh;' "
      "sandbox></iframe>");

  CompositeFrame();

  ExpectCount(WouldLoadReason::kCreated, 1);
  ExpectTotalCount(4);

  main_resource->Write("<script>theFrame.style.top='10px'</script>");
  main_resource->Finish();

  CompositeFrame();

  ExpectCount(WouldLoadReason::kVisible, 1);
  ExpectTotalCount(5);
}

TEST_F(DeferredLoadingTest, OneScreenBelowThenScrolledVisible) {
  std::unique_ptr<SimRequest> main_resource = CreateMainResource();

  main_resource->Complete(
      "<iframe id='theFrame' style='position:absolute; top:105vh; height:10px' "
      "sandbox></iframe>");

  CompositeFrame();

  ExpectCount(WouldLoadReason::kCreated, 1);
  ExpectCount(WouldLoadReason::k1ScreenAway, 1);
  ExpectCount(WouldLoadReason::k2ScreensAway, 1);
  ExpectCount(WouldLoadReason::k3ScreensAway, 1);
  ExpectTotalCount(4);

  MainFrame().SetScrollOffset(WebSize(0, 50));

  CompositeFrame();

  ExpectCount(WouldLoadReason::kVisible, 1);
  ExpectTotalCount(5);
}

TEST_F(DeferredLoadingTest, DisplayNoneThenTwoScreensAway) {
  std::unique_ptr<SimRequest> main_resource = CreateMainResource();

  main_resource->Start();
  main_resource->Write(
      "<iframe id='theFrame' style='display:none' sandbox></iframe>");

  CompositeFrame();

  ExpectCount(WouldLoadReason::kCreated, 1);
  ExpectTotalCount(6);

  main_resource->Write(R"HTML(
    <script>theFrame.style.top='200vh';
    theFrame.style.position='absolute';
    theFrame.style.display='block';</script>
  )HTML");
  main_resource->Finish();

  CompositeFrame();

  ExpectCount(WouldLoadReason::kNoParent, 1);
  ExpectTotalCount(6);
}

TEST_F(DeferredLoadingTest, DisplayNoneAsync) {
  std::unique_ptr<SimRequest> main_resource = CreateMainResource();

  main_resource->Start();
  main_resource->Write("some stuff");

  CompositeFrame();

  main_resource->Write(R"HTML(
    <script>frame = document.createElement('iframe');
    frame.setAttribute('sandbox', true);
    frame.style.display = 'none';
    document.body.appendChild(frame);
    </script>
  )HTML");
  main_resource->Finish();

  CompositeFrame();

  ExpectCount(WouldLoadReason::kCreated, 1);
  ExpectCount(WouldLoadReason::kNoParent, 1);
  ExpectTotalCount(6);
}

TEST_F(DeferredLoadingTest, TwoScreensAwayThenDisplayNoneThenNew) {
  std::unique_ptr<SimRequest> main_resource = CreateMainResource();

  main_resource->Start();
  main_resource->Write(
      "<iframe id='theFrame' style='position:absolute; top:205vh' sandbox>"
      "</iframe>");

  CompositeFrame();

  ExpectCount(WouldLoadReason::kCreated, 1);
  ExpectCount(WouldLoadReason::k2ScreensAway, 1);
  ExpectCount(WouldLoadReason::k3ScreensAway, 1);
  ExpectTotalCount(3);

  main_resource->Write("<script>theFrame.style.display='none'</script>");

  CompositeFrame();

  ExpectTotalCount(6);

  main_resource->Write(
      "<script>document.body.appendChild(document.createElement"
      "('iframe'));</script>");
  main_resource->Finish();

  CompositeFrame();

  ExpectCount(WouldLoadReason::kCreated, 1);
  ExpectCount(WouldLoadReason::kNoParent, 1);
  ExpectCount(WouldLoadReason::kVisible, 1);
  ExpectCount(WouldLoadReason::k1ScreenAway, 1);
  ExpectCount(WouldLoadReason::k2ScreensAway, 1);
  ExpectCount(WouldLoadReason::k3ScreensAway, 1);
  ExpectTotalCount(6);
}

TEST_F(DeferredLoadingTest, SameOriginNotCounted) {
  std::unique_ptr<SimRequest> main_resource = CreateMainResource();
  SimRequest frame_resource("https://example.com/iframe.html", "text/html");

  main_resource->Complete("<iframe src='iframe.html'></iframe>");
  frame_resource.Complete("<iframe></iframe>");
  CompositeFrame();

  ExpectTotalCount(0);
}

TEST_F(DeferredLoadingTest, AboveNestedInThreeScreensBelow) {
  std::unique_ptr<SimRequest> main_resource = CreateMainResource();
  SimRequest frame_resource("https://example.com/iframe.html", "text/html");

  main_resource->Complete(
      "<iframe style='position:absolute; top:300vh' src='iframe.html' "
      "sandbox></iframe>");
  frame_resource.Complete(
      "<iframe style='position:absolute; top:-10000px;' sandbox></iframe>");

  CompositeFrame();

  ExpectCount(WouldLoadReason::kCreated, 2);
  ExpectCount(WouldLoadReason::k3ScreensAway, 2);
  ExpectTotalCount(4);
}

TEST_F(DeferredLoadingTest, VisibleNestedInTwoScreensBelow) {
  std::unique_ptr<SimRequest> main_resource = CreateMainResource();
  SimRequest frame_resource("https://example.com/iframe.html", "text/html");

  main_resource->Complete(
      "<iframe style='position:absolute; top:205vh' src='iframe.html' "
      "sandbox></iframe>");
  frame_resource.Complete("<iframe sandbox></iframe>");

  CompositeFrame();

  ExpectCount(WouldLoadReason::kCreated, 2);
  ExpectCount(WouldLoadReason::k2ScreensAway, 2);
  ExpectCount(WouldLoadReason::k3ScreensAway, 2);
  ExpectTotalCount(6);
}

TEST_F(DeferredLoadingTest, ThreeScreensBelowNestedInTwoScreensBelow) {
  std::unique_ptr<SimRequest> main_resource = CreateMainResource();
  SimRequest frame_resource("https://example.com/iframe.html", "text/html");

  main_resource->Complete(
      "<iframe style='position:absolute; top:205vh' src='iframe.html' "
      "sandbox></iframe>");
  frame_resource.Complete(
      "<iframe style='position:absolute; top:305vh' sandbox></iframe>");

  CompositeFrame();

  ExpectCount(WouldLoadReason::kCreated, 2);
  ExpectCount(WouldLoadReason::k2ScreensAway, 1);
  ExpectCount(WouldLoadReason::k3ScreensAway, 1);
  ExpectTotalCount(4);
}

TEST_F(DeferredLoadingTest, TriplyNested) {
  std::unique_ptr<SimRequest> main_resource = CreateMainResource();
  SimRequest frame_resource("https://example.com/iframe.html", "text/html");
  SimRequest frame_resource2("https://example.com/iframe2.html", "text/html");

  main_resource->Complete(
      "<iframe style='position:absolute; top:300vh' src='iframe.html' "
      "sandbox></iframe>");
  frame_resource.Complete(
      "<iframe style='position:absolute; top:200vh' src='iframe2.html' "
      "sandbox></iframe>");
  frame_resource2.Complete(
      "<iframe style='position:absolute; top:100vh' sandbox></iframe>");

  CompositeFrame();

  ExpectCount(WouldLoadReason::kCreated, 3);
  ExpectCount(WouldLoadReason::k3ScreensAway, 1);
  ExpectTotalCount(4);
}

TEST_F(DeferredLoadingTest, NestedFramesOfVariousSizes) {
  std::unique_ptr<SimRequest> main_resource = CreateMainResource();
  SimRequest frame_resource("https://example.com/iframe.html", "text/html");
  SimRequest frame_resource2("https://example.com/iframe2.html", "text/html");
  SimRequest frame_resource3("https://example.com/iframe3.html", "text/html");

  main_resource->Complete(
      "<iframe style='position:absolute; top:50vh; height:10px;'"
      "src='iframe.html' sandbox></iframe>");
  frame_resource.Complete(
      "<iframe style='position:absolute; top:200vh; height:100px;'"
      "src='iframe2.html' sandbox></iframe>");
  frame_resource2.Complete(
      "<iframe style='position:absolute; top:100vh; height:50px;'"
      "src='iframe3.html' sandbox></iframe>");
  frame_resource3.Complete(
      "<iframe style='position:absolute; top:100vh' sandbox></iframe>");
  CompositeFrame();

  ExpectCount(WouldLoadReason::kCreated, 4);
  ExpectCount(WouldLoadReason::kVisible, 1);
  ExpectCount(WouldLoadReason::k1ScreenAway, 1);
  ExpectCount(WouldLoadReason::k2ScreensAway, 2);
  ExpectCount(WouldLoadReason::k3ScreensAway, 3);
  ExpectTotalCount(11);
}

TEST_F(DeferredLoadingTest, FourScreensBelow) {
  std::unique_ptr<SimRequest> main_resource = CreateMainResource();

  main_resource->Complete(
      "<iframe style='position:absolute; top:405vh;' sandbox></iframe>");

  CompositeFrame();

  ExpectCount(WouldLoadReason::kCreated, 1);
  ExpectTotalCount(1);
}

TEST_F(DeferredLoadingTest, TallIFrameStartsAbove) {
  std::unique_ptr<SimRequest> main_resource = CreateMainResource();

  main_resource->Complete(
      "<iframe style='position:absolute; top:-150vh; height:200vh;' sandbox>"
      "</iframe>");

  CompositeFrame();

  ExpectCount(WouldLoadReason::kCreated, 1);
  ExpectCount(WouldLoadReason::kVisible, 1);
  ExpectCount(WouldLoadReason::k1ScreenAway, 1);
  ExpectCount(WouldLoadReason::k2ScreensAway, 1);
  ExpectCount(WouldLoadReason::k3ScreensAway, 1);
  ExpectTotalCount(5);
}

TEST_F(DeferredLoadingTest, OneDownAndOneRight) {
  std::unique_ptr<SimRequest> main_resource = CreateMainResource();

  main_resource->Complete(
      "<iframe style='position:absolute; left:100vw; top:100vh' sandbox>"
      "</iframe>");

  CompositeFrame();

  ExpectCount(WouldLoadReason::kCreated, 1);
  ExpectTotalCount(1);
}

TEST_F(DeferredLoadingTest, VisibleCrossOriginNestedInBelowFoldSameOrigin) {
  std::unique_ptr<SimRequest> main_resource = CreateMainResource();
  SimRequest frame_resource("https://example.com/iframe.html", "text/html");

  main_resource->Complete(
      "<iframe style='position:absolute; top:105vh' src='iframe.html'>"
      "</iframe>");
  frame_resource.Complete("<iframe sandbox></iframe>");

  CompositeFrame();

  ExpectCount(WouldLoadReason::kCreated, 1);
  ExpectCount(WouldLoadReason::kVisible, 1);
  ExpectCount(WouldLoadReason::k1ScreenAway, 1);
  ExpectCount(WouldLoadReason::k2ScreensAway, 1);
  ExpectCount(WouldLoadReason::k3ScreensAway, 1);
  ExpectTotalCount(5);
}

TEST_F(DeferredLoadingTest, EmbedSVG) {
  std::unique_ptr<SimRequest> main_resource = CreateMainResource();
  SimRequest frame_resource("https://example.com/embed.svg", "image/svg+xml");
  main_resource->Complete("<iframe id='embed' src='embed.svg'></iframe>");
  CompositeFrame();
  Element* embed_element = GetDocument().getElementById("embed");
  embed_element->SetInlineStyleProperty(CSSPropertyDisplay, "none");
  frame_resource.Complete("<svg xmlns=\"http://www.w3.org/2000/svg\"></svg>");
}

}  // namespace blink
