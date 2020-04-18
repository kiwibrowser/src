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
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/page/scrolling/scrolling_coordinator.h"

#include "build/build_config.h"
#include "cc/layers/layer_sticky_position_constraint.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/web_cache.h"
#include "third_party/blink/public/platform/web_layer_tree_view.h"
#include "third_party/blink/public/platform/web_rect.h"
#include "third_party/blink/public/platform/web_url_loader_mock_factory.h"
#include "third_party/blink/public/web/web_settings.h"
#include "third_party/blink/public/web/web_view_client.h"
#include "third_party/blink/renderer/core/css/css_style_sheet.h"
#include "third_party/blink/renderer/core/css/style_sheet_list.h"
#include "third_party/blink/renderer/core/exported/web_view_impl.h"
#include "third_party/blink/renderer/core/frame/frame_test_helpers.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/frame/visual_viewport.h"
#include "third_party/blink/renderer/core/frame/web_frame_widget_base.h"
#include "third_party/blink/renderer/core/frame/web_local_frame_impl.h"
#include "third_party/blink/renderer/core/html/html_iframe_element.h"
#include "third_party/blink/renderer/core/layout/layout_embedded_content.h"
#include "third_party/blink/renderer/core/layout/layout_view.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/core/page/scrolling/scrolling_coordinator_context.h"
#include "third_party/blink/renderer/core/paint/compositing/composited_layer_mapping.h"
#include "third_party/blink/renderer/core/paint/compositing/paint_layer_compositor.h"
#include "third_party/blink/renderer/platform/geometry/int_point.h"
#include "third_party/blink/renderer/platform/geometry/int_rect.h"
#include "third_party/blink/renderer/platform/graphics/graphics_layer.h"
#include "third_party/blink/renderer/platform/graphics/touch_action.h"
#include "third_party/blink/renderer/platform/testing/histogram_tester.h"
#include "third_party/blink/renderer/platform/testing/unit_test_helpers.h"
#include "third_party/blink/renderer/platform/testing/url_test_helpers.h"

namespace blink {

class ScrollingCoordinatorTest : public testing::Test {
 public:
  ScrollingCoordinatorTest() : base_url_("http://www.test.com/") {
    helper_.Initialize(nullptr, nullptr, nullptr, &ConfigureSettings);
    GetWebView()->Resize(IntSize(320, 240));

    // macOS attaches main frame scrollbars to the VisualViewport so the
    // VisualViewport layers need to be initialized.
    GetWebView()->UpdateAllLifecyclePhases();
    WebFrameWidgetBase* main_frame_widget =
        GetWebView()->MainFrameImpl()->FrameWidgetImpl();
    main_frame_widget->SetRootGraphicsLayer(GetWebView()
                                                ->MainFrameImpl()
                                                ->GetFrame()
                                                ->View()
                                                ->GetLayoutView()
                                                ->Compositor()
                                                ->RootGraphicsLayer());
  }

  ~ScrollingCoordinatorTest() override {
    Platform::Current()
        ->GetURLLoaderMockFactory()
        ->UnregisterAllURLsAndClearMemoryCache();
  }

  void NavigateTo(const std::string& url) {
    FrameTestHelpers::LoadFrame(GetWebView()->MainFrameImpl(), url);
  }

  void LoadHTML(const std::string& html) {
    FrameTestHelpers::LoadHTMLString(GetWebView()->MainFrameImpl(), html,
                                     URLTestHelpers::ToKURL("about:blank"));
  }

  void ForceFullCompositingUpdate() {
    GetWebView()->UpdateAllLifecyclePhases();
  }

  void RegisterMockedHttpURLLoad(const std::string& file_name) {
    URLTestHelpers::RegisterMockedURLLoadFromBase(
        WebString::FromUTF8(base_url_), test::CoreTestDataPath(),
        WebString::FromUTF8(file_name));
  }

  cc::Layer* GetRootScrollLayer() {
    GraphicsLayer* layer =
        GetFrame()->View()->LayoutViewportScrollableArea()->LayerForScrolling();
    return layer ? layer->CcLayer() : nullptr;
  }

  WebViewImpl* GetWebView() const { return helper_.GetWebView(); }
  LocalFrame* GetFrame() const { return helper_.LocalMainFrame()->GetFrame(); }

  WebLayerTreeView* GetWebLayerTreeView() const {
    return GetWebView()->LayerTreeView();
  }

 protected:
  std::string base_url_;

 private:
  static void ConfigureSettings(WebSettings* settings) {
    settings->SetPreferCompositingToLCDTextEnabled(true);
  }

  FrameTestHelpers::WebViewHelper helper_;
};

TEST_F(ScrollingCoordinatorTest, fastScrollingByDefault) {
  GetWebView()->Resize(WebSize(800, 600));
  LoadHTML("<div id='spacer' style='height: 1000px'></div>");
  ForceFullCompositingUpdate();

  // Make sure the scrolling coordinator is active.
  LocalFrameView* frame_view = GetFrame()->View();
  Page* page = GetFrame()->GetPage();
  ASSERT_TRUE(page->GetScrollingCoordinator());
  ASSERT_TRUE(page->GetScrollingCoordinator()->CoordinatesScrollingForFrameView(
      frame_view));

  // Fast scrolling should be enabled by default.
  cc::Layer* root_scroll_layer = GetRootScrollLayer();
  ASSERT_TRUE(root_scroll_layer);
  ASSERT_TRUE(root_scroll_layer->scrollable());
  ASSERT_FALSE(root_scroll_layer->should_scroll_on_main_thread());
  ASSERT_EQ(WebEventListenerProperties::kNothing,
            GetWebLayerTreeView()->EventListenerProperties(
                WebEventListenerClass::kTouchStartOrMove));
  ASSERT_EQ(WebEventListenerProperties::kNothing,
            GetWebLayerTreeView()->EventListenerProperties(
                WebEventListenerClass::kMouseWheel));

  cc::Layer* inner_viewport_scroll_layer =
      page->GetVisualViewport().ScrollLayer()->CcLayer();
  ASSERT_TRUE(inner_viewport_scroll_layer->scrollable());
  ASSERT_FALSE(inner_viewport_scroll_layer->should_scroll_on_main_thread());
}

TEST_F(ScrollingCoordinatorTest, fastScrollingCanBeDisabledWithSetting) {
  GetWebView()->Resize(WebSize(800, 600));
  LoadHTML("<div id='spacer' style='height: 1000px'></div>");
  GetWebView()->GetSettings()->SetThreadedScrollingEnabled(false);
  ForceFullCompositingUpdate();

  // Make sure the scrolling coordinator is active.
  LocalFrameView* frame_view = GetFrame()->View();
  Page* page = GetFrame()->GetPage();
  ASSERT_TRUE(page->GetScrollingCoordinator());
  ASSERT_TRUE(page->GetScrollingCoordinator()->CoordinatesScrollingForFrameView(
      frame_view));

  // Main scrolling should be enabled with the setting override.
  cc::Layer* root_scroll_layer = GetRootScrollLayer();
  ASSERT_TRUE(root_scroll_layer);
  ASSERT_TRUE(root_scroll_layer->scrollable());
  ASSERT_TRUE(root_scroll_layer->should_scroll_on_main_thread());

  // Main scrolling should also propagate to inner viewport layer.
  cc::Layer* inner_viewport_scroll_layer =
      page->GetVisualViewport().ScrollLayer()->CcLayer();
  ASSERT_TRUE(inner_viewport_scroll_layer->scrollable());
  ASSERT_TRUE(inner_viewport_scroll_layer->should_scroll_on_main_thread());
}

TEST_F(ScrollingCoordinatorTest, fastFractionalScrollingDiv) {
  ScopedFractionalScrollOffsetsForTest fractional_scroll_offsets(true);

  RegisterMockedHttpURLLoad("fractional-scroll-div.html");
  NavigateTo(base_url_ + "fractional-scroll-div.html");
  ForceFullCompositingUpdate();

  Document* document = GetFrame()->GetDocument();
  Element* scrollable_element = document->getElementById("scroller");
  DCHECK(scrollable_element);

  scrollable_element->setScrollTop(1.0);
  scrollable_element->setScrollLeft(1.0);
  ForceFullCompositingUpdate();

  // Make sure the fractional scroll offset change 1.0 -> 1.2 gets propagated
  // to compositor.
  scrollable_element->setScrollTop(1.2);
  scrollable_element->setScrollLeft(1.2);
  ForceFullCompositingUpdate();

  LayoutObject* layout_object = scrollable_element->GetLayoutObject();
  ASSERT_TRUE(layout_object->IsBox());
  LayoutBox* box = ToLayoutBox(layout_object);
  ASSERT_TRUE(box->UsesCompositedScrolling());
  CompositedLayerMapping* composited_layer_mapping =
      box->Layer()->GetCompositedLayerMapping();
  ASSERT_TRUE(composited_layer_mapping->HasScrollingLayer());
  DCHECK(composited_layer_mapping->ScrollingContentsLayer());
  cc::Layer* cc_scroll_layer =
      composited_layer_mapping->ScrollingContentsLayer()->CcLayer();
  ASSERT_TRUE(cc_scroll_layer);
  ASSERT_NEAR(1.2f, cc_scroll_layer->scroll_offset().x(), 0.01f);
  ASSERT_NEAR(1.2f, cc_scroll_layer->scroll_offset().y(), 0.01f);
}

static cc::Layer* CcLayerFromElement(Element* element) {
  if (!element)
    return nullptr;
  LayoutObject* layout_object = element->GetLayoutObject();
  if (!layout_object || !layout_object->IsBoxModelObject())
    return nullptr;
  PaintLayer* layer = ToLayoutBoxModelObject(layout_object)->Layer();
  if (!layer)
    return nullptr;
  if (!layer->HasCompositedLayerMapping())
    return nullptr;
  CompositedLayerMapping* composited_layer_mapping =
      layer->GetCompositedLayerMapping();
  GraphicsLayer* graphics_layer = composited_layer_mapping->MainGraphicsLayer();
  if (!graphics_layer)
    return nullptr;
  return graphics_layer->CcLayer();
}

TEST_F(ScrollingCoordinatorTest, fastScrollingForFixedPosition) {
  RegisterMockedHttpURLLoad("fixed-position.html");
  NavigateTo(base_url_ + "fixed-position.html");
  ForceFullCompositingUpdate();

  // Fixed position should not fall back to main thread scrolling.
  cc::Layer* root_scroll_layer = GetRootScrollLayer();
  ASSERT_TRUE(root_scroll_layer);
  ASSERT_FALSE(root_scroll_layer->should_scroll_on_main_thread());

  Document* document = GetFrame()->GetDocument();
  {
    Element* element = document->getElementById("div-tl");
    ASSERT_TRUE(element);
    cc::Layer* layer = CcLayerFromElement(element);
    ASSERT_TRUE(layer);
    cc::LayerPositionConstraint constraint = layer->position_constraint();
    ASSERT_TRUE(constraint.is_fixed_position());
    ASSERT_TRUE(!constraint.is_fixed_to_right_edge() &&
                !constraint.is_fixed_to_bottom_edge());
  }
  {
    Element* element = document->getElementById("div-tr");
    ASSERT_TRUE(element);
    cc::Layer* layer = CcLayerFromElement(element);
    ASSERT_TRUE(layer);
    cc::LayerPositionConstraint constraint = layer->position_constraint();
    ASSERT_TRUE(constraint.is_fixed_position());
    ASSERT_TRUE(constraint.is_fixed_to_right_edge() &&
                !constraint.is_fixed_to_bottom_edge());
  }
  {
    Element* element = document->getElementById("div-bl");
    ASSERT_TRUE(element);
    cc::Layer* layer = CcLayerFromElement(element);
    ASSERT_TRUE(layer);
    cc::LayerPositionConstraint constraint = layer->position_constraint();
    ASSERT_TRUE(constraint.is_fixed_position());
    ASSERT_TRUE(!constraint.is_fixed_to_right_edge() &&
                constraint.is_fixed_to_bottom_edge());
  }
  {
    Element* element = document->getElementById("div-br");
    ASSERT_TRUE(element);
    cc::Layer* layer = CcLayerFromElement(element);
    ASSERT_TRUE(layer);
    cc::LayerPositionConstraint constraint = layer->position_constraint();
    ASSERT_TRUE(constraint.is_fixed_position());
    ASSERT_TRUE(constraint.is_fixed_to_right_edge() &&
                constraint.is_fixed_to_bottom_edge());
  }
  {
    Element* element = document->getElementById("span-tl");
    ASSERT_TRUE(element);
    cc::Layer* layer = CcLayerFromElement(element);
    ASSERT_TRUE(layer);
    cc::LayerPositionConstraint constraint = layer->position_constraint();
    ASSERT_TRUE(constraint.is_fixed_position());
    ASSERT_TRUE(!constraint.is_fixed_to_right_edge() &&
                !constraint.is_fixed_to_bottom_edge());
  }
  {
    Element* element = document->getElementById("span-tr");
    ASSERT_TRUE(element);
    cc::Layer* layer = CcLayerFromElement(element);
    ASSERT_TRUE(layer);
    cc::LayerPositionConstraint constraint = layer->position_constraint();
    ASSERT_TRUE(constraint.is_fixed_position());
    ASSERT_TRUE(constraint.is_fixed_to_right_edge() &&
                !constraint.is_fixed_to_bottom_edge());
  }
  {
    Element* element = document->getElementById("span-bl");
    ASSERT_TRUE(element);
    cc::Layer* layer = CcLayerFromElement(element);
    ASSERT_TRUE(layer);
    cc::LayerPositionConstraint constraint = layer->position_constraint();
    ASSERT_TRUE(constraint.is_fixed_position());
    ASSERT_TRUE(!constraint.is_fixed_to_right_edge() &&
                constraint.is_fixed_to_bottom_edge());
  }
  {
    Element* element = document->getElementById("span-br");
    ASSERT_TRUE(element);
    cc::Layer* layer = CcLayerFromElement(element);
    ASSERT_TRUE(layer);
    cc::LayerPositionConstraint constraint = layer->position_constraint();
    ASSERT_TRUE(constraint.is_fixed_position());
    ASSERT_TRUE(constraint.is_fixed_to_right_edge() &&
                constraint.is_fixed_to_bottom_edge());
  }
}

TEST_F(ScrollingCoordinatorTest, fastScrollingForStickyPosition) {
  RegisterMockedHttpURLLoad("sticky-position.html");
  NavigateTo(base_url_ + "sticky-position.html");
  ForceFullCompositingUpdate();

  // Sticky position should not fall back to main thread scrolling.
  cc::Layer* root_scroll_layer = GetRootScrollLayer();
  ASSERT_TRUE(root_scroll_layer);
  EXPECT_FALSE(root_scroll_layer->should_scroll_on_main_thread());

  Document* document = GetFrame()->GetDocument();
  {
    Element* element = document->getElementById("div-tl");
    ASSERT_TRUE(element);
    cc::Layer* layer = CcLayerFromElement(element);
    ASSERT_TRUE(layer);
    cc::LayerStickyPositionConstraint constraint =
        layer->sticky_position_constraint();
    ASSERT_TRUE(constraint.is_sticky);
    EXPECT_TRUE(constraint.is_anchored_top && constraint.is_anchored_left &&
                !constraint.is_anchored_right &&
                !constraint.is_anchored_bottom);
    EXPECT_EQ(1.f, constraint.top_offset);
    EXPECT_EQ(1.f, constraint.left_offset);
    EXPECT_EQ(gfx::Rect(100, 100, 10, 10),
              constraint.scroll_container_relative_sticky_box_rect);
    EXPECT_EQ(gfx::Rect(100, 100, 200, 200),
              constraint.scroll_container_relative_containing_block_rect);
  }
  {
    Element* element = document->getElementById("div-tr");
    ASSERT_TRUE(element);
    cc::Layer* layer = CcLayerFromElement(element);
    ASSERT_TRUE(layer);
    cc::LayerStickyPositionConstraint constraint =
        layer->sticky_position_constraint();
    ASSERT_TRUE(constraint.is_sticky);
    EXPECT_TRUE(constraint.is_anchored_top && !constraint.is_anchored_left &&
                constraint.is_anchored_right && !constraint.is_anchored_bottom);
  }
  {
    Element* element = document->getElementById("div-bl");
    ASSERT_TRUE(element);
    cc::Layer* layer = CcLayerFromElement(element);
    ASSERT_TRUE(layer);
    cc::LayerStickyPositionConstraint constraint =
        layer->sticky_position_constraint();
    ASSERT_TRUE(constraint.is_sticky);
    EXPECT_TRUE(!constraint.is_anchored_top && constraint.is_anchored_left &&
                !constraint.is_anchored_right && constraint.is_anchored_bottom);
  }
  {
    Element* element = document->getElementById("div-br");
    ASSERT_TRUE(element);
    cc::Layer* layer = CcLayerFromElement(element);
    ASSERT_TRUE(layer);
    cc::LayerStickyPositionConstraint constraint =
        layer->sticky_position_constraint();
    ASSERT_TRUE(constraint.is_sticky);
    EXPECT_TRUE(!constraint.is_anchored_top && !constraint.is_anchored_left &&
                constraint.is_anchored_right && constraint.is_anchored_bottom);
  }
  {
    Element* element = document->getElementById("span-tl");
    ASSERT_TRUE(element);
    cc::Layer* layer = CcLayerFromElement(element);
    ASSERT_TRUE(layer);
    cc::LayerStickyPositionConstraint constraint =
        layer->sticky_position_constraint();
    ASSERT_TRUE(constraint.is_sticky);
    EXPECT_TRUE(constraint.is_anchored_top && constraint.is_anchored_left &&
                !constraint.is_anchored_right &&
                !constraint.is_anchored_bottom);
  }
  {
    Element* element = document->getElementById("span-tlbr");
    ASSERT_TRUE(element);
    cc::Layer* layer = CcLayerFromElement(element);
    ASSERT_TRUE(layer);
    cc::LayerStickyPositionConstraint constraint =
        layer->sticky_position_constraint();
    ASSERT_TRUE(constraint.is_sticky);
    EXPECT_TRUE(constraint.is_anchored_top && constraint.is_anchored_left &&
                constraint.is_anchored_right && constraint.is_anchored_bottom);
    EXPECT_EQ(1.f, constraint.top_offset);
    EXPECT_EQ(1.f, constraint.left_offset);
    EXPECT_EQ(1.f, constraint.right_offset);
    EXPECT_EQ(1.f, constraint.bottom_offset);
  }
  {
    Element* element = document->getElementById("composited-top");
    ASSERT_TRUE(element);
    cc::Layer* layer = CcLayerFromElement(element);
    ASSERT_TRUE(layer);
    cc::LayerStickyPositionConstraint constraint =
        layer->sticky_position_constraint();
    ASSERT_TRUE(constraint.is_sticky);
    EXPECT_TRUE(constraint.is_anchored_top);
    EXPECT_EQ(gfx::Rect(100, 110, 10, 10),
              constraint.scroll_container_relative_sticky_box_rect);
    EXPECT_EQ(gfx::Rect(100, 100, 200, 200),
              constraint.scroll_container_relative_containing_block_rect);
  }
}

TEST_F(ScrollingCoordinatorTest, touchEventHandler) {
  RegisterMockedHttpURLLoad("touch-event-handler.html");
  NavigateTo(base_url_ + "touch-event-handler.html");
  ForceFullCompositingUpdate();

  ASSERT_EQ(WebEventListenerProperties::kBlocking,
            GetWebLayerTreeView()->EventListenerProperties(
                WebEventListenerClass::kTouchStartOrMove));
}

TEST_F(ScrollingCoordinatorTest, touchEventHandlerPassive) {
  RegisterMockedHttpURLLoad("touch-event-handler-passive.html");
  NavigateTo(base_url_ + "touch-event-handler-passive.html");
  ForceFullCompositingUpdate();

  ASSERT_EQ(WebEventListenerProperties::kPassive,
            GetWebLayerTreeView()->EventListenerProperties(
                WebEventListenerClass::kTouchStartOrMove));
}

TEST_F(ScrollingCoordinatorTest, touchEventHandlerBoth) {
  RegisterMockedHttpURLLoad("touch-event-handler-both.html");
  NavigateTo(base_url_ + "touch-event-handler-both.html");
  ForceFullCompositingUpdate();

  ASSERT_EQ(WebEventListenerProperties::kBlockingAndPassive,
            GetWebLayerTreeView()->EventListenerProperties(
                WebEventListenerClass::kTouchStartOrMove));
}

TEST_F(ScrollingCoordinatorTest, wheelEventHandler) {
  RegisterMockedHttpURLLoad("wheel-event-handler.html");
  NavigateTo(base_url_ + "wheel-event-handler.html");
  ForceFullCompositingUpdate();

  ASSERT_EQ(WebEventListenerProperties::kBlocking,
            GetWebLayerTreeView()->EventListenerProperties(
                WebEventListenerClass::kMouseWheel));
}

TEST_F(ScrollingCoordinatorTest, wheelEventHandlerPassive) {
  RegisterMockedHttpURLLoad("wheel-event-handler-passive.html");
  NavigateTo(base_url_ + "wheel-event-handler-passive.html");
  ForceFullCompositingUpdate();

  ASSERT_EQ(WebEventListenerProperties::kPassive,
            GetWebLayerTreeView()->EventListenerProperties(
                WebEventListenerClass::kMouseWheel));
}

TEST_F(ScrollingCoordinatorTest, wheelEventHandlerBoth) {
  RegisterMockedHttpURLLoad("wheel-event-handler-both.html");
  NavigateTo(base_url_ + "wheel-event-handler-both.html");
  ForceFullCompositingUpdate();

  ASSERT_EQ(WebEventListenerProperties::kBlockingAndPassive,
            GetWebLayerTreeView()->EventListenerProperties(
                WebEventListenerClass::kMouseWheel));
}

TEST_F(ScrollingCoordinatorTest, scrollEventHandler) {
  RegisterMockedHttpURLLoad("scroll-event-handler.html");
  NavigateTo(base_url_ + "scroll-event-handler.html");
  ForceFullCompositingUpdate();

  ASSERT_TRUE(GetWebLayerTreeView()->HaveScrollEventHandlers());
}

TEST_F(ScrollingCoordinatorTest, updateEventHandlersDuringTeardown) {
  RegisterMockedHttpURLLoad("scroll-event-handler-window.html");
  NavigateTo(base_url_ + "scroll-event-handler-window.html");
  ForceFullCompositingUpdate();

  // Simulate detaching the document from its DOM window. This should not
  // cause a crash when the WebViewImpl is closed by the test runner.
  GetFrame()->GetDocument()->Shutdown();
}

TEST_F(ScrollingCoordinatorTest, clippedBodyTest) {
  RegisterMockedHttpURLLoad("clipped-body.html");
  NavigateTo(base_url_ + "clipped-body.html");
  ForceFullCompositingUpdate();

  cc::Layer* root_scroll_layer = GetRootScrollLayer();
  ASSERT_TRUE(root_scroll_layer);
  EXPECT_TRUE(root_scroll_layer->non_fast_scrollable_region().IsEmpty());
}

TEST_F(ScrollingCoordinatorTest, touchAction) {
  RegisterMockedHttpURLLoad("touch-action.html");
  NavigateTo(base_url_ + "touch-action.html");
  ForceFullCompositingUpdate();

  Element* scrollable_element =
      GetFrame()->GetDocument()->getElementById("scrollable");
  LayoutBox* box = ToLayoutBox(scrollable_element->GetLayoutObject());
  ASSERT_TRUE(box->UsesCompositedScrolling());
  ASSERT_EQ(kPaintsIntoOwnBacking, box->Layer()->GetCompositingState());

  CompositedLayerMapping* composited_layer_mapping =
      box->Layer()->GetCompositedLayerMapping();

  GraphicsLayer* graphics_layer = composited_layer_mapping->MainGraphicsLayer();
  cc::Layer* cc_layer = graphics_layer->CcLayer();
  cc::Region region = cc_layer->touch_action_region().GetRegionForTouchAction(
      TouchAction::kTouchActionPanX | TouchAction::kTouchActionPanDown);
  EXPECT_EQ(region.GetRegionComplexity(), 1);
  EXPECT_EQ(region.bounds(), gfx::Rect(0, 0, 1000, 1000));
}

TEST_F(ScrollingCoordinatorTest, touchActionRegions) {
  RegisterMockedHttpURLLoad("touch-action-regions.html");
  NavigateTo(base_url_ + "touch-action-regions.html");
  ForceFullCompositingUpdate();

  Element* scrollable_element =
      GetFrame()->GetDocument()->getElementById("scrollable");
  LayoutBox* box = ToLayoutBox(scrollable_element->GetLayoutObject());
  ASSERT_TRUE(box->UsesCompositedScrolling());
  ASSERT_EQ(kPaintsIntoOwnBacking, box->Layer()->GetCompositingState());

  CompositedLayerMapping* composited_layer_mapping =
      box->Layer()->GetCompositedLayerMapping();

  GraphicsLayer* graphics_layer = composited_layer_mapping->MainGraphicsLayer();
  cc::Layer* cc_layer = graphics_layer->CcLayer();

  cc::Region region = cc_layer->touch_action_region().GetRegionForTouchAction(
      TouchAction::kTouchActionPanDown | TouchAction::kTouchActionPanX);
  EXPECT_EQ(region.GetRegionComplexity(), 1);
  EXPECT_EQ(region.bounds(), gfx::Rect(0, 0, 100, 100));

  region = cc_layer->touch_action_region().GetRegionForTouchAction(
      TouchAction::kTouchActionPanDown | TouchAction::kTouchActionPanRight);
  EXPECT_EQ(region.GetRegionComplexity(), 1);
  EXPECT_EQ(region.bounds(), gfx::Rect(0, 0, 50, 50));

  region = cc_layer->touch_action_region().GetRegionForTouchAction(
      TouchAction::kTouchActionPanDown);
  EXPECT_EQ(region.GetRegionComplexity(), 1);
  EXPECT_EQ(region.bounds(), gfx::Rect(0, 100, 100, 100));
}

TEST_F(ScrollingCoordinatorTest, touchActionBlockingHandler) {
  RegisterMockedHttpURLLoad("touch-action-blocking-handler.html");
  NavigateTo(base_url_ + "touch-action-blocking-handler.html");
  ForceFullCompositingUpdate();

  Element* scrollable_element =
      GetFrame()->GetDocument()->getElementById("scrollable");
  LayoutBox* box = ToLayoutBox(scrollable_element->GetLayoutObject());
  ASSERT_TRUE(box->UsesCompositedScrolling());
  ASSERT_EQ(kPaintsIntoOwnBacking, box->Layer()->GetCompositingState());

  CompositedLayerMapping* composited_layer_mapping =
      box->Layer()->GetCompositedLayerMapping();

  GraphicsLayer* graphics_layer = composited_layer_mapping->MainGraphicsLayer();
  cc::Layer* cc_layer = graphics_layer->CcLayer();

  cc::Region region = cc_layer->touch_action_region().GetRegionForTouchAction(
      TouchAction::kTouchActionNone);
  EXPECT_EQ(region.GetRegionComplexity(), 1);
  EXPECT_EQ(region.bounds(), gfx::Rect(0, 0, 100, 100));

  region = cc_layer->touch_action_region().GetRegionForTouchAction(
      TouchAction::kTouchActionPanY);
  EXPECT_EQ(region.GetRegionComplexity(), 1);
  EXPECT_EQ(region.bounds(), gfx::Rect(0, 0, 1000, 1000));
}

TEST_F(ScrollingCoordinatorTest, IframeWindowTouchHandler) {
  LoadHTML(
      R"(<iframe style="width: 275px; height: 250px;"></iframe>)");
  WebLocalFrameImpl* child_frame =
      ToWebLocalFrameImpl(GetWebView()->MainFrameImpl()->FirstChild());
  FrameTestHelpers::LoadHTMLString(child_frame,
                                   R"(<body>
                                   <p style="margin: 1000px"> Hello </p>
        <script>
          window.addEventListener('touchstart', (e) => {
            e.preventDefault();
          }, {passive: false});
        </script>
      </body>)",
                                   URLTestHelpers::ToKURL("about:blank"));
  ForceFullCompositingUpdate();

  PaintLayer* paint_layer_child_frame =
      child_frame->GetFrame()->GetDocument()->GetLayoutView()->Layer();
  cc::Region region_child_frame =
      paint_layer_child_frame
          ->EnclosingLayerForPaintInvalidationCrossingFrameBoundaries()
          ->GraphicsLayerBacking(&paint_layer_child_frame->GetLayoutObject())
          ->CcLayer()
          ->touch_action_region()
          .GetRegionForTouchAction(TouchAction::kTouchActionNone);
  PaintLayer* paint_layer_main_frame = GetWebView()
                                           ->MainFrameImpl()
                                           ->GetFrame()
                                           ->GetDocument()
                                           ->GetLayoutView()
                                           ->Layer();
  cc::Region region_main_frame =
      paint_layer_main_frame
          ->EnclosingLayerForPaintInvalidationCrossingFrameBoundaries()
          ->GraphicsLayerBacking(&paint_layer_main_frame->GetLayoutObject())
          ->CcLayer()
          ->touch_action_region()
          .GetRegionForTouchAction(TouchAction::kTouchActionNone);
  EXPECT_TRUE(region_main_frame.bounds().IsEmpty());
  EXPECT_FALSE(region_child_frame.bounds().IsEmpty());
  // We only check for the content size for verification as the offset is 0x0
  // due to child frame having its own composited layer.
  EXPECT_EQ(child_frame->GetFrameView()->ContentsSize(),
            IntRect(region_child_frame.bounds()).Size());
}

TEST_F(ScrollingCoordinatorTest, overflowScrolling) {
  RegisterMockedHttpURLLoad("overflow-scrolling.html");
  NavigateTo(base_url_ + "overflow-scrolling.html");
  ForceFullCompositingUpdate();

  // Verify the properties of the accelerated scrolling element starting from
  // the LayoutObject all the way to the cc::Layer.
  Element* scrollable_element =
      GetFrame()->GetDocument()->getElementById("scrollable");
  DCHECK(scrollable_element);

  LayoutObject* layout_object = scrollable_element->GetLayoutObject();
  ASSERT_TRUE(layout_object->IsBox());
  ASSERT_TRUE(layout_object->HasLayer());

  LayoutBox* box = ToLayoutBox(layout_object);
  ASSERT_TRUE(box->UsesCompositedScrolling());
  ASSERT_EQ(kPaintsIntoOwnBacking, box->Layer()->GetCompositingState());

  CompositedLayerMapping* composited_layer_mapping =
      box->Layer()->GetCompositedLayerMapping();
  ASSERT_TRUE(composited_layer_mapping->HasScrollingLayer());
  DCHECK(composited_layer_mapping->ScrollingContentsLayer());

  GraphicsLayer* graphics_layer =
      composited_layer_mapping->ScrollingContentsLayer();
  ASSERT_EQ(box->Layer()->GetScrollableArea(),
            graphics_layer->GetScrollableArea());

  cc::Layer* cc_scroll_layer =
      composited_layer_mapping->ScrollingContentsLayer()->CcLayer();
  ASSERT_TRUE(cc_scroll_layer->scrollable());
  ASSERT_TRUE(cc_scroll_layer->user_scrollable_horizontal());
  ASSERT_TRUE(cc_scroll_layer->user_scrollable_vertical());

#if defined(OS_ANDROID)
  // Now verify we've attached impl-side scrollbars onto the scrollbar layers
  ASSERT_TRUE(composited_layer_mapping->LayerForHorizontalScrollbar());
  ASSERT_TRUE(composited_layer_mapping->LayerForHorizontalScrollbar()
                  ->HasContentsLayer());
  ASSERT_TRUE(composited_layer_mapping->LayerForVerticalScrollbar());
  ASSERT_TRUE(composited_layer_mapping->LayerForVerticalScrollbar()
                  ->HasContentsLayer());
#endif
}

TEST_F(ScrollingCoordinatorTest, overflowHidden) {
  RegisterMockedHttpURLLoad("overflow-hidden.html");
  NavigateTo(base_url_ + "overflow-hidden.html");
  ForceFullCompositingUpdate();

  // Verify the properties of the accelerated scrolling element starting from
  // the LayoutObject all the way to the cc::Layer.
  Element* overflow_element =
      GetFrame()->GetDocument()->getElementById("unscrollable-y");
  DCHECK(overflow_element);

  LayoutObject* layout_object = overflow_element->GetLayoutObject();
  ASSERT_TRUE(layout_object->IsBox());
  ASSERT_TRUE(layout_object->HasLayer());

  LayoutBox* box = ToLayoutBox(layout_object);
  ASSERT_TRUE(box->UsesCompositedScrolling());
  ASSERT_EQ(kPaintsIntoOwnBacking, box->Layer()->GetCompositingState());

  CompositedLayerMapping* composited_layer_mapping =
      box->Layer()->GetCompositedLayerMapping();
  ASSERT_TRUE(composited_layer_mapping->HasScrollingLayer());
  DCHECK(composited_layer_mapping->ScrollingContentsLayer());

  GraphicsLayer* graphics_layer =
      composited_layer_mapping->ScrollingContentsLayer();
  ASSERT_EQ(box->Layer()->GetScrollableArea(),
            graphics_layer->GetScrollableArea());

  cc::Layer* cc_scroll_layer =
      composited_layer_mapping->ScrollingContentsLayer()->CcLayer();
  ASSERT_TRUE(cc_scroll_layer->scrollable());
  ASSERT_TRUE(cc_scroll_layer->user_scrollable_horizontal());
  ASSERT_FALSE(cc_scroll_layer->user_scrollable_vertical());

  overflow_element =
      GetFrame()->GetDocument()->getElementById("unscrollable-x");
  DCHECK(overflow_element);

  layout_object = overflow_element->GetLayoutObject();
  ASSERT_TRUE(layout_object->IsBox());
  ASSERT_TRUE(layout_object->HasLayer());

  box = ToLayoutBox(layout_object);
  ASSERT_TRUE(box->GetScrollableArea()->UsesCompositedScrolling());
  ASSERT_EQ(kPaintsIntoOwnBacking, box->Layer()->GetCompositingState());

  composited_layer_mapping = box->Layer()->GetCompositedLayerMapping();
  ASSERT_TRUE(composited_layer_mapping->HasScrollingLayer());
  DCHECK(composited_layer_mapping->ScrollingContentsLayer());

  graphics_layer = composited_layer_mapping->ScrollingContentsLayer();
  ASSERT_EQ(box->Layer()->GetScrollableArea(),
            graphics_layer->GetScrollableArea());

  cc_scroll_layer =
      composited_layer_mapping->ScrollingContentsLayer()->CcLayer();
  ASSERT_TRUE(cc_scroll_layer->scrollable());
  ASSERT_FALSE(cc_scroll_layer->user_scrollable_horizontal());
  ASSERT_TRUE(cc_scroll_layer->user_scrollable_vertical());
}

TEST_F(ScrollingCoordinatorTest, iframeScrolling) {
  RegisterMockedHttpURLLoad("iframe-scrolling.html");
  RegisterMockedHttpURLLoad("iframe-scrolling-inner.html");
  NavigateTo(base_url_ + "iframe-scrolling.html");
  ForceFullCompositingUpdate();

  // Verify the properties of the accelerated scrolling element starting from
  // the LayoutObject all the way to the cc::Layer.
  Element* scrollable_frame =
      GetFrame()->GetDocument()->getElementById("scrollable");
  ASSERT_TRUE(scrollable_frame);

  LayoutObject* layout_object = scrollable_frame->GetLayoutObject();
  ASSERT_TRUE(layout_object);
  ASSERT_TRUE(layout_object->IsLayoutEmbeddedContent());

  LayoutEmbeddedContent* layout_embedded_content =
      ToLayoutEmbeddedContent(layout_object);
  ASSERT_TRUE(layout_embedded_content);

  LocalFrameView* inner_frame_view =
      ToLocalFrameView(layout_embedded_content->ChildFrameView());
  ASSERT_TRUE(inner_frame_view);

  auto* inner_layout_view = inner_frame_view->GetLayoutView();
  ASSERT_TRUE(inner_layout_view);

  PaintLayerCompositor* inner_compositor = inner_layout_view->Compositor();
  ASSERT_TRUE(inner_compositor->InCompositingMode());

  GraphicsLayer* scroll_layer =
      inner_frame_view->LayoutViewportScrollableArea()->LayerForScrolling();
  ASSERT_TRUE(scroll_layer);
  ASSERT_EQ(inner_frame_view->LayoutViewportScrollableArea(),
            scroll_layer->GetScrollableArea());

  cc::Layer* cc_scroll_layer = scroll_layer->CcLayer();
  ASSERT_TRUE(cc_scroll_layer->scrollable());

#if defined(OS_ANDROID)
  // Now verify we've attached impl-side scrollbars onto the scrollbar layers
  GraphicsLayer* horizontal_scrollbar_layer =
      inner_frame_view->LayoutViewportScrollableArea()
          ->LayerForHorizontalScrollbar();
  ASSERT_TRUE(horizontal_scrollbar_layer);
  ASSERT_TRUE(horizontal_scrollbar_layer->HasContentsLayer());
  GraphicsLayer* vertical_scrollbar_layer =
      inner_frame_view->LayoutViewportScrollableArea()
          ->LayerForVerticalScrollbar();
  ASSERT_TRUE(vertical_scrollbar_layer);
  ASSERT_TRUE(vertical_scrollbar_layer->HasContentsLayer());
#endif
}

TEST_F(ScrollingCoordinatorTest, rtlIframe) {
  RegisterMockedHttpURLLoad("rtl-iframe.html");
  RegisterMockedHttpURLLoad("rtl-iframe-inner.html");
  NavigateTo(base_url_ + "rtl-iframe.html");
  ForceFullCompositingUpdate();

  // Verify the properties of the accelerated scrolling element starting from
  // the LayoutObject all the way to the cc::Layer.
  Element* scrollable_frame =
      GetFrame()->GetDocument()->getElementById("scrollable");
  ASSERT_TRUE(scrollable_frame);

  LayoutObject* layout_object = scrollable_frame->GetLayoutObject();
  ASSERT_TRUE(layout_object);
  ASSERT_TRUE(layout_object->IsLayoutEmbeddedContent());

  LayoutEmbeddedContent* layout_embedded_content =
      ToLayoutEmbeddedContent(layout_object);
  ASSERT_TRUE(layout_embedded_content);

  LocalFrameView* inner_frame_view =
      ToLocalFrameView(layout_embedded_content->ChildFrameView());
  ASSERT_TRUE(inner_frame_view);

  auto* inner_layout_view = inner_frame_view->GetLayoutView();
  ASSERT_TRUE(inner_layout_view);

  PaintLayerCompositor* inner_compositor = inner_layout_view->Compositor();
  ASSERT_TRUE(inner_compositor->InCompositingMode());

  GraphicsLayer* scroll_layer =
      inner_frame_view->LayoutViewportScrollableArea()->LayerForScrolling();
  ASSERT_TRUE(scroll_layer);
  ASSERT_EQ(inner_frame_view->LayoutViewportScrollableArea(),
            scroll_layer->GetScrollableArea());

  cc::Layer* cc_scroll_layer = scroll_layer->CcLayer();
  ASSERT_TRUE(cc_scroll_layer->scrollable());

  int expected_scroll_position =
      958 + (inner_frame_view->LayoutViewportScrollableArea()
                     ->VerticalScrollbar()
                     ->IsOverlayScrollbar()
                 ? 0
                 : 15);
  ASSERT_EQ(expected_scroll_position, cc_scroll_layer->scroll_offset().x());
}

TEST_F(ScrollingCoordinatorTest, setupScrollbarLayerShouldNotCrash) {
  RegisterMockedHttpURLLoad("setup_scrollbar_layer_crash.html");
  NavigateTo(base_url_ + "setup_scrollbar_layer_crash.html");
  ForceFullCompositingUpdate();
  // This test document setup an iframe with scrollbars, then switch to
  // an empty document by javascript.
}

TEST_F(ScrollingCoordinatorTest,
       ScrollbarsForceMainThreadOrHaveCompositorScrollbarLayer) {
  RegisterMockedHttpURLLoad("trivial-scroller.html");
  NavigateTo(base_url_ + "trivial-scroller.html");
  ForceFullCompositingUpdate();

  Document* document = GetFrame()->GetDocument();
  Element* scrollable_element = document->getElementById("scroller");
  DCHECK(scrollable_element);

  LayoutObject* layout_object = scrollable_element->GetLayoutObject();
  ASSERT_TRUE(layout_object->IsBox());
  LayoutBox* box = ToLayoutBox(layout_object);
  ASSERT_TRUE(box->UsesCompositedScrolling());
  CompositedLayerMapping* composited_layer_mapping =
      box->Layer()->GetCompositedLayerMapping();
  GraphicsLayer* scrollbar_graphics_layer =
      composited_layer_mapping->LayerForVerticalScrollbar();
  ASSERT_TRUE(scrollbar_graphics_layer);

  bool has_cc_scrollbar_layer = !scrollbar_graphics_layer->DrawsContent();
  ASSERT_TRUE(
      has_cc_scrollbar_layer ||
      scrollbar_graphics_layer->CcLayer()->should_scroll_on_main_thread());
}

#if defined(OS_MACOSX) || defined(OS_ANDROID)
TEST_F(ScrollingCoordinatorTest,
       DISABLED_setupScrollbarLayerShouldSetScrollLayerOpaque)
#else
TEST_F(ScrollingCoordinatorTest, setupScrollbarLayerShouldSetScrollLayerOpaque)
#endif
{
  RegisterMockedHttpURLLoad("wide_document.html");
  NavigateTo(base_url_ + "wide_document.html");
  ForceFullCompositingUpdate();

  LocalFrameView* frame_view = GetFrame()->View();
  ASSERT_TRUE(frame_view);

  GraphicsLayer* scrollbar_graphics_layer =
      frame_view->LayoutViewportScrollableArea()->LayerForHorizontalScrollbar();
  ASSERT_TRUE(scrollbar_graphics_layer);

  cc::Layer* platform_layer = scrollbar_graphics_layer->CcLayer();
  ASSERT_TRUE(platform_layer);

  cc::Layer* contents_layer = scrollbar_graphics_layer->ContentsLayer();
  ASSERT_TRUE(contents_layer);

  // After ScrollableAreaScrollbarLayerDidChange(),
  // if the main frame's scrollbar_layer is opaque,
  // contents_layer should be opaque too.
  ASSERT_EQ(platform_layer->contents_opaque(),
            contents_layer->contents_opaque());
}

TEST_F(ScrollingCoordinatorTest,
       FixedPositionLosingBackingShouldTriggerMainThreadScroll) {
  GetWebView()->GetSettings()->SetPreferCompositingToLCDTextEnabled(false);
  RegisterMockedHttpURLLoad("fixed-position-losing-backing.html");
  NavigateTo(base_url_ + "fixed-position-losing-backing.html");
  ForceFullCompositingUpdate();

  cc::Layer* scroll_layer = GetRootScrollLayer();
  ASSERT_TRUE(scroll_layer);

  Document* document = GetFrame()->GetDocument();
  Element* fixed_pos = document->getElementById("fixed");

  EXPECT_TRUE(static_cast<LayoutBoxModelObject*>(fixed_pos->GetLayoutObject())
                  ->Layer()
                  ->HasCompositedLayerMapping());
  EXPECT_FALSE(scroll_layer->should_scroll_on_main_thread());

  fixed_pos->SetInlineStyleProperty(CSSPropertyTransform, CSSValueNone);
  ForceFullCompositingUpdate();

  EXPECT_FALSE(static_cast<LayoutBoxModelObject*>(fixed_pos->GetLayoutObject())
                   ->Layer()
                   ->HasCompositedLayerMapping());
  EXPECT_TRUE(scroll_layer->should_scroll_on_main_thread());
}

TEST_F(ScrollingCoordinatorTest, CustomScrollbarShouldTriggerMainThreadScroll) {
  GetWebView()->GetSettings()->SetPreferCompositingToLCDTextEnabled(true);
  GetWebView()->SetDeviceScaleFactor(2.f);
  RegisterMockedHttpURLLoad("custom_scrollbar.html");
  NavigateTo(base_url_ + "custom_scrollbar.html");
  ForceFullCompositingUpdate();

  Document* document = GetFrame()->GetDocument();
  Element* container = document->getElementById("container");
  Element* content = document->getElementById("content");
  DCHECK_EQ(container->getAttribute(HTMLNames::classAttr), "custom_scrollbar");
  DCHECK(container);
  DCHECK(content);

  LayoutObject* layout_object = container->GetLayoutObject();
  ASSERT_TRUE(layout_object->IsBox());
  LayoutBox* box = ToLayoutBox(layout_object);
  ASSERT_TRUE(box->UsesCompositedScrolling());
  CompositedLayerMapping* composited_layer_mapping =
      box->Layer()->GetCompositedLayerMapping();
  GraphicsLayer* scrollbar_graphics_layer =
      composited_layer_mapping->LayerForVerticalScrollbar();
  ASSERT_TRUE(scrollbar_graphics_layer);
  ASSERT_TRUE(
      scrollbar_graphics_layer->CcLayer()->should_scroll_on_main_thread());
  ASSERT_TRUE(
      scrollbar_graphics_layer->CcLayer()->main_thread_scrolling_reasons() &
      MainThreadScrollingReason::kCustomScrollbarScrolling);

  // remove custom scrollbar class, the scrollbar is expected to scroll on
  // impl thread as it is an overlay scrollbar.
  container->removeAttribute("class");
  ForceFullCompositingUpdate();
  scrollbar_graphics_layer =
      composited_layer_mapping->LayerForVerticalScrollbar();
  ASSERT_FALSE(
      scrollbar_graphics_layer->CcLayer()->should_scroll_on_main_thread());
  ASSERT_FALSE(
      scrollbar_graphics_layer->CcLayer()->main_thread_scrolling_reasons() &
      MainThreadScrollingReason::kCustomScrollbarScrolling);
}

TEST_F(ScrollingCoordinatorTest,
       BackgroundAttachmentFixedShouldTriggerMainThreadScroll) {
  RegisterMockedHttpURLLoad("iframe-background-attachment-fixed.html");
  RegisterMockedHttpURLLoad("iframe-background-attachment-fixed-inner.html");
  RegisterMockedHttpURLLoad("white-1x1.png");
  NavigateTo(base_url_ + "iframe-background-attachment-fixed.html");
  ForceFullCompositingUpdate();

  Element* iframe = GetFrame()->GetDocument()->getElementById("iframe");
  ASSERT_TRUE(iframe);

  LayoutObject* layout_object = iframe->GetLayoutObject();
  ASSERT_TRUE(layout_object);
  ASSERT_TRUE(layout_object->IsLayoutEmbeddedContent());

  LayoutEmbeddedContent* layout_embedded_content =
      ToLayoutEmbeddedContent(layout_object);
  ASSERT_TRUE(layout_embedded_content);

  LocalFrameView* inner_frame_view =
      ToLocalFrameView(layout_embedded_content->ChildFrameView());
  ASSERT_TRUE(inner_frame_view);

  auto* inner_layout_view = inner_frame_view->GetLayoutView();
  ASSERT_TRUE(inner_layout_view);

  PaintLayerCompositor* inner_compositor = inner_layout_view->Compositor();
  ASSERT_TRUE(inner_compositor->InCompositingMode());

  GraphicsLayer* scroll_layer =
      inner_frame_view->LayoutViewportScrollableArea()->LayerForScrolling();
  ASSERT_TRUE(scroll_layer);
  ASSERT_EQ(inner_frame_view->LayoutViewportScrollableArea(),
            scroll_layer->GetScrollableArea());

  cc::Layer* cc_scroll_layer = scroll_layer->CcLayer();
  ASSERT_TRUE(cc_scroll_layer->scrollable());
  ASSERT_TRUE(cc_scroll_layer->main_thread_scrolling_reasons() &
              MainThreadScrollingReason::kHasBackgroundAttachmentFixedObjects);

  // Remove fixed background-attachment should make the iframe
  // scroll on cc.
  auto* iframe_doc = ToHTMLIFrameElement(iframe)->contentDocument();
  iframe = iframe_doc->getElementById("scrollable");
  ASSERT_TRUE(iframe);

  iframe->removeAttribute("class");
  ForceFullCompositingUpdate();

  layout_object = iframe->GetLayoutObject();
  ASSERT_TRUE(layout_object);

  scroll_layer = layout_object->GetFrameView()
                     ->LayoutViewportScrollableArea()
                     ->LayerForScrolling();
  ASSERT_TRUE(scroll_layer);

  cc_scroll_layer = scroll_layer->CcLayer();
  ASSERT_TRUE(cc_scroll_layer->scrollable());
  ASSERT_FALSE(cc_scroll_layer->main_thread_scrolling_reasons() &
               MainThreadScrollingReason::kHasBackgroundAttachmentFixedObjects);

  // Force main frame to scroll on main thread. All its descendants
  // should scroll on main thread as well.
  Element* element = GetFrame()->GetDocument()->getElementById("scrollable");
  element->setAttribute(
      "style",
      "background-image: url('white-1x1.png'); background-attachment: fixed;",
      ASSERT_NO_EXCEPTION);

  ForceFullCompositingUpdate();

  layout_object = iframe->GetLayoutObject();
  ASSERT_TRUE(layout_object);

  scroll_layer = layout_object->GetFrameView()
                     ->LayoutViewportScrollableArea()
                     ->LayerForScrolling();
  ASSERT_TRUE(scroll_layer);

  cc_scroll_layer = scroll_layer->CcLayer();
  ASSERT_TRUE(cc_scroll_layer->scrollable());
  ASSERT_TRUE(cc_scroll_layer->main_thread_scrolling_reasons() &
              MainThreadScrollingReason::kHasBackgroundAttachmentFixedObjects);
}

// Upon resizing the content size, the main thread scrolling reason
// kHasNonLayerViewportConstrainedObject should be updated on all frames
TEST_F(ScrollingCoordinatorTest,
       RecalculateMainThreadScrollingReasonsUponResize) {
  GetWebView()->GetSettings()->SetPreferCompositingToLCDTextEnabled(false);
  RegisterMockedHttpURLLoad("has-non-layer-viewport-constrained-objects.html");
  NavigateTo(base_url_ + "has-non-layer-viewport-constrained-objects.html");
  ForceFullCompositingUpdate();

  Element* element = GetFrame()->GetDocument()->getElementById("scrollable");
  ASSERT_TRUE(element);

  LayoutObject* layout_object = element->GetLayoutObject();
  ASSERT_TRUE(layout_object);

  // When the div becomes to scrollable it should scroll on main thread
  element->setAttribute("style",
                        "overflow:scroll;height:2000px;will-change:transform;",
                        ASSERT_NO_EXCEPTION);
  ForceFullCompositingUpdate();

  layout_object = element->GetLayoutObject();
  ASSERT_TRUE(layout_object);

  GraphicsLayer* scroll_layer = layout_object->GetFrameView()
                                    ->LayoutViewportScrollableArea()
                                    ->LayerForScrolling();
  ASSERT_TRUE(scroll_layer);

  cc::Layer* cc_scroll_layer = scroll_layer->CcLayer();
  ASSERT_TRUE(cc_scroll_layer->scrollable());
  ASSERT_TRUE(
      cc_scroll_layer->main_thread_scrolling_reasons() &
      MainThreadScrollingReason::kHasNonLayerViewportConstrainedObjects);

  // The main thread scrolling reason should be reset upon the following change
  element->setAttribute("style",
                        "overflow:scroll;height:200px;will-change:transform;",
                        ASSERT_NO_EXCEPTION);
  ForceFullCompositingUpdate();

  layout_object = element->GetLayoutObject();
  ASSERT_TRUE(layout_object);
}

TEST_F(ScrollingCoordinatorTest, StickyTriggersMainThreadScroll) {
  GetWebView()->GetSettings()->SetPreferCompositingToLCDTextEnabled(false);
  LoadHTML(
      "<body style='height: 1200px'>"
      "<div style='position: sticky; top: 0'>sticky</div>");
  ForceFullCompositingUpdate();
  ScrollableArea* viewport = GetFrame()->View()->LayoutViewportScrollableArea();
  cc::Layer* scroll_layer = viewport->LayerForScrolling()->CcLayer();
  ASSERT_EQ(MainThreadScrollingReason::kHasNonLayerViewportConstrainedObjects,
            scroll_layer->main_thread_scrolling_reasons());
}

// LocalFrameView::FrameIsScrollableDidChange is used as a dirty bit and is
// set to clean in ScrollingCoordinator::UpdateAfterCompositingChangeIfNeeded.
// This test ensures that the dirty bit is set and unset properly.
TEST_F(ScrollingCoordinatorTest, FrameIsScrollableDidChange) {
  LoadHTML(R"HTML(
    <div id='bg' style='background: red; width: 10px; height: 10px;'></div>
    <div id='forcescroll' style='height: 5000px;'></div>
  )HTML");

  // Initially there is a change but that goes away after a compositing update.
  EXPECT_TRUE(GetFrame()->View()->FrameIsScrollableDidChange());
  ForceFullCompositingUpdate();
  EXPECT_FALSE(GetFrame()->View()->FrameIsScrollableDidChange());

  // A change to background color should not change the frame's scrollability.
  auto* background = GetFrame()->GetDocument()->getElementById("bg");
  background->removeAttribute(HTMLNames::styleAttr);
  EXPECT_FALSE(GetFrame()->View()->FrameIsScrollableDidChange());

  ForceFullCompositingUpdate();

  // Making the frame not scroll should change the frame's scrollability.
  auto* forcescroll = GetFrame()->GetDocument()->getElementById("forcescroll");
  forcescroll->removeAttribute(HTMLNames::styleAttr);
  GetFrame()->View()->UpdateLifecycleToLayoutClean();
  EXPECT_TRUE(GetFrame()->View()->FrameIsScrollableDidChange());

  ForceFullCompositingUpdate();
  EXPECT_FALSE(GetFrame()->View()->FrameIsScrollableDidChange());
}

TEST_F(ScrollingCoordinatorTest, UpdateUMAMetricUpdated) {
  HistogramTester histogram_tester;
  LoadHTML(R"HTML(
    <div id='bg' style='background: blue;'></div>
    <div id='scroller' style='overflow: scroll; width: 10px; height: 10px;'>
      <div id='forcescroll' style='height: 1000px;'></div>
    </div>
  )HTML");

  // The initial count should be zero.
  histogram_tester.ExpectTotalCount("Blink.ScrollingCoordinator.UpdateTime", 0);

  // After an initial compositing update, we should have one scrolling update.
  ForceFullCompositingUpdate();
  histogram_tester.ExpectTotalCount("Blink.ScrollingCoordinator.UpdateTime", 1);

  // An update with no scrolling changes should not cause a scrolling update.
  ForceFullCompositingUpdate();
  histogram_tester.ExpectTotalCount("Blink.ScrollingCoordinator.UpdateTime", 1);

  // A change to background color should not cause a scrolling update.
  auto* background = GetFrame()->GetDocument()->getElementById("bg");
  background->removeAttribute(HTMLNames::styleAttr);
  ForceFullCompositingUpdate();
  histogram_tester.ExpectTotalCount("Blink.ScrollingCoordinator.UpdateTime", 1);

  // Removing a scrollable area should cause a scrolling update.
  auto* scroller = GetFrame()->GetDocument()->getElementById("scroller");
  scroller->removeAttribute(HTMLNames::styleAttr);
  ForceFullCompositingUpdate();
  histogram_tester.ExpectTotalCount("Blink.ScrollingCoordinator.UpdateTime", 2);
}

class NonCompositedMainThreadScrollingReasonTest
    : public ScrollingCoordinatorTest {
  static const uint32_t kLCDTextRelatedReasons =
      MainThreadScrollingReason::kHasOpacityAndLCDText |
      MainThreadScrollingReason::kHasTransformAndLCDText |
      MainThreadScrollingReason::kBackgroundNotOpaqueInRectAndLCDText |
      MainThreadScrollingReason::kIsNotStackingContextAndLCDText;

 protected:
  NonCompositedMainThreadScrollingReasonTest() {
    RegisterMockedHttpURLLoad("two_scrollable_area.html");
    NavigateTo(base_url_ + "two_scrollable_area.html");
  }
  void TestNonCompositedReasons(const std::string& target,
                                const uint32_t reason) {
    GetWebView()->GetSettings()->SetPreferCompositingToLCDTextEnabled(false);
    Document* document = GetFrame()->GetDocument();
    Element* container = document->getElementById("scroller1");
    container->setAttribute("class", target.c_str(), ASSERT_NO_EXCEPTION);
    ForceFullCompositingUpdate();

    PaintLayerScrollableArea* scrollable_area =
        ToLayoutBoxModelObject(container->GetLayoutObject())
            ->GetScrollableArea();
    ASSERT_TRUE(scrollable_area);
    EXPECT_TRUE(scrollable_area->GetNonCompositedMainThreadScrollingReasons() &
                reason);

    Element* container2 = document->getElementById("scroller2");
    PaintLayerScrollableArea* scrollable_area2 =
        ToLayoutBoxModelObject(container2->GetLayoutObject())
            ->GetScrollableArea();
    ASSERT_TRUE(scrollable_area2);
    // Different scrollable area should remain unaffected.
    EXPECT_FALSE(
        scrollable_area2->GetNonCompositedMainThreadScrollingReasons() &
        reason);

    LocalFrameView* frame_view = GetFrame()->View();
    ASSERT_TRUE(frame_view);
    EXPECT_FALSE(frame_view->GetMainThreadScrollingReasons() & reason);

    // Remove attribute from the scroller 1 would lead to scroll on impl.
    container->removeAttribute("class");
    ForceFullCompositingUpdate();

    EXPECT_FALSE(scrollable_area->GetNonCompositedMainThreadScrollingReasons() &
                 reason);
    EXPECT_FALSE(frame_view->GetMainThreadScrollingReasons() & reason);

    // Add target attribute would again lead to scroll on main thread
    container->setAttribute("class", target.c_str(), ASSERT_NO_EXCEPTION);
    ForceFullCompositingUpdate();

    EXPECT_TRUE(scrollable_area->GetNonCompositedMainThreadScrollingReasons() &
                reason);
    EXPECT_FALSE(frame_view->GetMainThreadScrollingReasons() & reason);

    if ((reason & kLCDTextRelatedReasons) &&
        !(reason & ~kLCDTextRelatedReasons)) {
      GetWebView()->GetSettings()->SetPreferCompositingToLCDTextEnabled(true);
      ForceFullCompositingUpdate();
      EXPECT_FALSE(
          scrollable_area->GetNonCompositedMainThreadScrollingReasons());
      EXPECT_FALSE(frame_view->GetMainThreadScrollingReasons());
    }
  }
};

TEST_F(NonCompositedMainThreadScrollingReasonTest, TransparentTest) {
  TestNonCompositedReasons("transparent",
                           MainThreadScrollingReason::kHasOpacityAndLCDText);
}

TEST_F(NonCompositedMainThreadScrollingReasonTest, TransformTest) {
  TestNonCompositedReasons("transform",
                           MainThreadScrollingReason::kHasTransformAndLCDText);
}

TEST_F(NonCompositedMainThreadScrollingReasonTest, BackgroundNotOpaqueTest) {
  TestNonCompositedReasons(
      "background-not-opaque",
      MainThreadScrollingReason::kBackgroundNotOpaqueInRectAndLCDText);
}

TEST_F(NonCompositedMainThreadScrollingReasonTest, ClipTest) {
  TestNonCompositedReasons("clip",
                           MainThreadScrollingReason::kHasClipRelatedProperty);
}

TEST_F(NonCompositedMainThreadScrollingReasonTest, ClipPathTest) {
  uint32_t clip_reason = MainThreadScrollingReason::kHasClipRelatedProperty;
  GetWebView()->GetSettings()->SetPreferCompositingToLCDTextEnabled(false);
  Document* document = GetFrame()->GetDocument();
  // Test ancestor with ClipPath
  Element* element = document->body();
  ASSERT_TRUE(element);
  element->setAttribute(HTMLNames::styleAttr,
                        "clip-path:circle(115px at 20px 20px);");
  Element* container = document->getElementById("scroller1");
  ASSERT_TRUE(container);
  ForceFullCompositingUpdate();

  PaintLayerScrollableArea* scrollable_area =
      ToLayoutBoxModelObject(container->GetLayoutObject())->GetScrollableArea();
  ASSERT_TRUE(scrollable_area);
  EXPECT_TRUE(scrollable_area->GetNonCompositedMainThreadScrollingReasons() &
              clip_reason);

  LocalFrameView* frame_view = GetFrame()->View();
  ASSERT_TRUE(frame_view);
  EXPECT_FALSE(frame_view->GetMainThreadScrollingReasons() & clip_reason);

  // Remove clip path from ancestor.
  element->removeAttribute(HTMLNames::styleAttr);
  ForceFullCompositingUpdate();

  EXPECT_FALSE(scrollable_area->GetNonCompositedMainThreadScrollingReasons() &
               clip_reason);
  EXPECT_FALSE(frame_view->GetMainThreadScrollingReasons() & clip_reason);

  // Test descendant with ClipPath
  element = document->getElementById("content1");
  ASSERT_TRUE(element);
  element->setAttribute(HTMLNames::styleAttr,
                        "clip-path:circle(115px at 20px 20px);");
  ForceFullCompositingUpdate();
  EXPECT_TRUE(scrollable_area->GetNonCompositedMainThreadScrollingReasons() &
              clip_reason);
  EXPECT_FALSE(frame_view->GetMainThreadScrollingReasons() & clip_reason);

  // Remove clip path from descendant.
  element->removeAttribute(HTMLNames::styleAttr);
  ForceFullCompositingUpdate();
  EXPECT_FALSE(scrollable_area->GetNonCompositedMainThreadScrollingReasons() &
               clip_reason);
  EXPECT_FALSE(frame_view->GetMainThreadScrollingReasons() & clip_reason);
}

TEST_F(NonCompositedMainThreadScrollingReasonTest, LCDTextEnabledTest) {
  TestNonCompositedReasons("transparent",
                           MainThreadScrollingReason::kHasOpacityAndLCDText);
}

TEST_F(NonCompositedMainThreadScrollingReasonTest, BoxShadowTest) {
  TestNonCompositedReasons(
      "box-shadow", MainThreadScrollingReason::kHasBoxShadowFromNonRootLayer);
}

TEST_F(NonCompositedMainThreadScrollingReasonTest, StackingContextTest) {
  GetWebView()->GetSettings()->SetPreferCompositingToLCDTextEnabled(false);

  Document* document = GetFrame()->GetDocument();
  Element* container = document->getElementById("scroller1");
  ASSERT_TRUE(container);

  ForceFullCompositingUpdate();

  // If a scroller contains all its children, it's not a stacking context.
  PaintLayerScrollableArea* scrollable_area =
      ToLayoutBoxModelObject(container->GetLayoutObject())->GetScrollableArea();
  ASSERT_TRUE(scrollable_area);
  EXPECT_TRUE(scrollable_area->GetNonCompositedMainThreadScrollingReasons() &
              MainThreadScrollingReason::kIsNotStackingContextAndLCDText);

  GetWebView()->GetSettings()->SetPreferCompositingToLCDTextEnabled(true);
  ForceFullCompositingUpdate();
  EXPECT_FALSE(scrollable_area->GetNonCompositedMainThreadScrollingReasons() &
               MainThreadScrollingReason::kIsNotStackingContextAndLCDText);
  GetWebView()->GetSettings()->SetPreferCompositingToLCDTextEnabled(false);

  // Adding "contain: paint" to force a stacking context leads to promotion.
  container->setAttribute("style", "contain: paint", ASSERT_NO_EXCEPTION);
  ForceFullCompositingUpdate();

  EXPECT_FALSE(scrollable_area->GetNonCompositedMainThreadScrollingReasons());
}

TEST_F(NonCompositedMainThreadScrollingReasonTest,
       CompositedWithLCDTextRelatedReasonsTest) {
  // With "will-change:transform" we composite elements with
  // LCDTextRelatedReasons only. For elements with other
  // NonCompositedReasons, we don't create scrollingLayer for their
  // CompositedLayerMapping therefore they don't get composited.
  GetWebView()->GetSettings()->SetPreferCompositingToLCDTextEnabled(false);
  Document* document = GetFrame()->GetDocument();
  Element* container = document->getElementById("scroller1");
  ASSERT_TRUE(container);
  container->setAttribute("class", "composited transparent",
                          ASSERT_NO_EXCEPTION);
  ForceFullCompositingUpdate();

  PaintLayerScrollableArea* scrollable_area =
      ToLayoutBoxModelObject(container->GetLayoutObject())->GetScrollableArea();
  ASSERT_TRUE(scrollable_area);
  EXPECT_FALSE(scrollable_area->GetNonCompositedMainThreadScrollingReasons());

  Element* container2 = document->getElementById("scroller2");
  ASSERT_TRUE(container2);
  container2->setAttribute("class", "composited border-radius",
                           ASSERT_NO_EXCEPTION);
  ForceFullCompositingUpdate();
  PaintLayerScrollableArea* scrollable_area2 =
      ToLayoutBoxModelObject(container2->GetLayoutObject())
          ->GetScrollableArea();
  ASSERT_TRUE(scrollable_area2);
  ASSERT_TRUE(scrollable_area2->UsesCompositedScrolling());
}

}  // namespace blink
