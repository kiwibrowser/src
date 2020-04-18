/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#include "third_party/blink/renderer/core/paint/link_highlight_impl.h"

#include <memory>

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/web_float_point.h"
#include "third_party/blink/public/platform/web_input_event.h"
#include "third_party/blink/public/platform/web_size.h"
#include "third_party/blink/public/platform/web_url_loader_mock_factory.h"
#include "third_party/blink/public/web/web_frame.h"
#include "third_party/blink/public/web/web_frame_client.h"
#include "third_party/blink/public/web/web_view_client.h"
#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/core/dom/node.h"
#include "third_party/blink/renderer/core/events/web_input_event_conversion.h"
#include "third_party/blink/renderer/core/exported/web_view_impl.h"
#include "third_party/blink/renderer/core/frame/frame_test_helpers.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/frame/web_local_frame_impl.h"
#include "third_party/blink/renderer/core/input/event_handler.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/core/page/touch_disambiguation.h"
#include "third_party/blink/renderer/platform/geometry/int_rect.h"
#include "third_party/blink/renderer/platform/graphics/graphics_layer.h"
#include "third_party/blink/renderer/platform/testing/unit_test_helpers.h"
#include "third_party/blink/renderer/platform/testing/url_test_helpers.h"

namespace blink {

namespace {

GestureEventWithHitTestResults GetTargetedEvent(WebViewImpl* web_view_impl,
                                                WebGestureEvent& touch_event) {
  WebGestureEvent scaled_event = TransformWebGestureEvent(
      web_view_impl->MainFrameImpl()->GetFrameView(), touch_event);
  return web_view_impl->GetPage()
      ->DeprecatedLocalMainFrame()
      ->GetEventHandler()
      .TargetGestureEvent(scaled_event, true);
}

std::string LinkRegisterMockedURLLoad() {
  WebURL url = URLTestHelpers::RegisterMockedURLLoadFromBase(
      WebString::FromUTF8("http://www.test.com/"), test::CoreTestDataPath(),
      WebString::FromUTF8("test_touch_link_highlight.html"));
  return url.GetString().Utf8();
}

}  // namespace

TEST(LinkHighlightImplTest, verifyWebViewImplIntegration) {
  const std::string url = LinkRegisterMockedURLLoad();
  FrameTestHelpers::WebViewHelper web_view_helper;
  WebViewImpl* web_view_impl = web_view_helper.InitializeAndLoad(url);
  int page_width = 640;
  int page_height = 480;
  web_view_impl->Resize(WebSize(page_width, page_height));
  web_view_impl->UpdateAllLifecyclePhases();

  WebGestureEvent touch_event(WebInputEvent::kGestureShowPress,
                              WebInputEvent::kNoModifiers,
                              WebInputEvent::GetStaticTimeStampForTests(),
                              kWebGestureDeviceTouchscreen);

  // The coordinates below are linked to absolute positions in the referenced
  // .html file.
  touch_event.SetPositionInWidget(WebFloatPoint(20, 20));

  ASSERT_TRUE(
      web_view_impl->BestTapNode(GetTargetedEvent(web_view_impl, touch_event)));

  touch_event.SetPositionInWidget(WebFloatPoint(20, 40));
  EXPECT_FALSE(
      web_view_impl->BestTapNode(GetTargetedEvent(web_view_impl, touch_event)));

  touch_event.SetPositionInWidget(WebFloatPoint(20, 20));
  // Shouldn't crash.
  web_view_impl->EnableTapHighlightAtPoint(
      GetTargetedEvent(web_view_impl, touch_event));

  EXPECT_TRUE(web_view_impl->GetLinkHighlight(0));
  EXPECT_TRUE(web_view_impl->GetLinkHighlight(0)->ContentLayer());
  EXPECT_TRUE(web_view_impl->GetLinkHighlight(0)->ClipLayer());

  // Find a target inside a scrollable div
  touch_event.SetPositionInWidget(WebFloatPoint(20, 100));
  web_view_impl->EnableTapHighlightAtPoint(
      GetTargetedEvent(web_view_impl, touch_event));
  ASSERT_TRUE(web_view_impl->GetLinkHighlight(0));

  // Don't highlight if no "hand cursor"
  touch_event.SetPositionInWidget(
      WebFloatPoint(20, 220));  // An A-link with cross-hair cursor.
  web_view_impl->EnableTapHighlightAtPoint(
      GetTargetedEvent(web_view_impl, touch_event));
  ASSERT_EQ(0U, web_view_impl->NumLinkHighlights());

  touch_event.SetPositionInWidget(WebFloatPoint(20, 260));  // A text input box.
  web_view_impl->EnableTapHighlightAtPoint(
      GetTargetedEvent(web_view_impl, touch_event));
  ASSERT_EQ(0U, web_view_impl->NumLinkHighlights());

  Platform::Current()
      ->GetURLLoaderMockFactory()
      ->UnregisterAllURLsAndClearMemoryCache();
}

TEST(LinkHighlightImplTest, resetDuringNodeRemoval) {
  const std::string url = LinkRegisterMockedURLLoad();
  FrameTestHelpers::WebViewHelper web_view_helper;
  WebViewImpl* web_view_impl = web_view_helper.InitializeAndLoad(url);

  int page_width = 640;
  int page_height = 480;
  web_view_impl->Resize(WebSize(page_width, page_height));
  web_view_impl->UpdateAllLifecyclePhases();

  WebGestureEvent touch_event(WebInputEvent::kGestureShowPress,
                              WebInputEvent::kNoModifiers,
                              WebInputEvent::GetStaticTimeStampForTests(),
                              kWebGestureDeviceTouchscreen);
  touch_event.SetPositionInWidget(WebFloatPoint(20, 20));

  GestureEventWithHitTestResults targeted_event =
      GetTargetedEvent(web_view_impl, touch_event);
  Node* touch_node = web_view_impl->BestTapNode(targeted_event);
  ASSERT_TRUE(touch_node);

  web_view_impl->EnableTapHighlightAtPoint(targeted_event);
  ASSERT_TRUE(web_view_impl->GetLinkHighlight(0));

  GraphicsLayer* highlight_layer =
      web_view_impl->GetLinkHighlight(0)->CurrentGraphicsLayerForTesting();
  ASSERT_TRUE(highlight_layer);
  EXPECT_TRUE(highlight_layer->GetLinkHighlight(0));

  touch_node->remove(IGNORE_EXCEPTION_FOR_TESTING);
  web_view_impl->UpdateAllLifecyclePhases();
  ASSERT_EQ(0U, highlight_layer->NumLinkHighlights());

  Platform::Current()
      ->GetURLLoaderMockFactory()
      ->UnregisterAllURLsAndClearMemoryCache();
}

// A lifetime test: delete LayerTreeView while running LinkHighlights.
TEST(LinkHighlightImplTest, resetLayerTreeView) {
  const std::string url = LinkRegisterMockedURLLoad();
  FrameTestHelpers::WebViewHelper web_view_helper;
  WebViewImpl* web_view_impl = web_view_helper.InitializeAndLoad(url);

  int page_width = 640;
  int page_height = 480;
  web_view_impl->Resize(WebSize(page_width, page_height));
  web_view_impl->UpdateAllLifecyclePhases();

  WebGestureEvent touch_event(WebInputEvent::kGestureShowPress,
                              WebInputEvent::kNoModifiers,
                              WebInputEvent::GetStaticTimeStampForTests(),
                              kWebGestureDeviceTouchscreen);
  touch_event.SetPositionInWidget(WebFloatPoint(20, 20));

  GestureEventWithHitTestResults targeted_event =
      GetTargetedEvent(web_view_impl, touch_event);
  Node* touch_node = web_view_impl->BestTapNode(targeted_event);
  ASSERT_TRUE(touch_node);

  web_view_impl->EnableTapHighlightAtPoint(targeted_event);
  ASSERT_TRUE(web_view_impl->GetLinkHighlight(0));

  GraphicsLayer* highlight_layer =
      web_view_impl->GetLinkHighlight(0)->CurrentGraphicsLayerForTesting();
  ASSERT_TRUE(highlight_layer);
  EXPECT_TRUE(highlight_layer->GetLinkHighlight(0));

  Platform::Current()
      ->GetURLLoaderMockFactory()
      ->UnregisterAllURLsAndClearMemoryCache();
}

TEST(LinkHighlightImplTest, multipleHighlights) {
  const std::string url = LinkRegisterMockedURLLoad();
  FrameTestHelpers::WebViewHelper web_view_helper;
  WebViewImpl* web_view_impl = web_view_helper.InitializeAndLoad(url);

  int page_width = 640;
  int page_height = 480;
  web_view_impl->Resize(WebSize(page_width, page_height));
  web_view_impl->UpdateAllLifecyclePhases();

  WebGestureEvent touch_event;
  touch_event.SetPositionInWidget(WebFloatPoint(50, 310));
  touch_event.data.tap.width = 30;
  touch_event.data.tap.height = 30;

  Vector<IntRect> good_targets;
  HeapVector<Member<Node>> highlight_nodes;
  IntRect bounding_box(
      touch_event.PositionInWidget().x - touch_event.data.tap.width / 2,
      touch_event.PositionInWidget().y - touch_event.data.tap.height / 2,
      touch_event.data.tap.width, touch_event.data.tap.height);
  FindGoodTouchTargets(bounding_box, web_view_impl->MainFrameImpl()->GetFrame(),
                       good_targets, highlight_nodes);

  web_view_impl->EnableTapHighlights(highlight_nodes);
  EXPECT_EQ(2U, web_view_impl->NumLinkHighlights());

  Platform::Current()
      ->GetURLLoaderMockFactory()
      ->UnregisterAllURLsAndClearMemoryCache();
}

}  // namespace blink
