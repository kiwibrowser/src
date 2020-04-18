// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "components/ui_devtools/css_agent.h"
#include "components/ui_devtools/ui_element.h"
#include "components/ui_devtools/views/dom_agent_aura.h"
#include "components/ui_devtools/views/overlay_agent_aura.h"
#include "components/ui_devtools/views/view_element.h"
#include "components/ui_devtools/views/widget_element.h"
#include "components/ui_devtools/views/window_element.h"
#include "ui/aura/client/window_parenting_client.h"
#include "ui/aura/window_tree_host.h"
#include "ui/display/display.h"
#include "ui/events/event_constants.h"
#include "ui/events/test/event_generator.h"
#include "ui/views/background.h"
#include "ui/views/test/views_test_base.h"
#include "ui/views/widget/native_widget_private.h"
#include "ui/views/widget/widget.h"
#include "ui/wm/core/coordinate_conversion.h"

namespace ui_devtools {
namespace {

using namespace ui_devtools::protocol;

const int kDefaultChildNodeCount = -1;
const SkColor kBackgroundColor = 0;
const SkColor kBorderColor = SK_ColorBLUE;

class TestView : public views::View {
 public:
  TestView(const char* name) : views::View(), name_(name) {}

  const char* GetClassName() const override { return name_; }

 private:
  const char* name_;

  DISALLOW_COPY_AND_ASSIGN(TestView);
};

class FakeFrontendChannel : public FrontendChannel {
 public:
  FakeFrontendChannel() {}
  ~FakeFrontendChannel() override {}

  int CountProtocolNotificationMessageStartsWith(const std::string& message) {
    int count = 0;
    for (const std::string& s : protocol_notification_messages_) {
      if (base::StartsWith(s, message, base::CompareCase::SENSITIVE))
        count++;
    }
    return count;
  }

  int CountProtocolNotificationMessage(const std::string& message) {
    return std::count(protocol_notification_messages_.begin(),
                      protocol_notification_messages_.end(), message);
  }

  // FrontendChannel:
  void sendProtocolResponse(int callId,
                            std::unique_ptr<Serializable> message) override {}
  void flushProtocolNotifications() override {}
  void sendProtocolNotification(
      std::unique_ptr<Serializable> message) override {
    protocol_notification_messages_.push_back(message->serialize());
  }

 private:
  std::vector<std::string> protocol_notification_messages_;

  DISALLOW_COPY_AND_ASSIGN(FakeFrontendChannel);
};

std::string GetAttributeValue(const std::string& attribute, DOM::Node* node) {
  EXPECT_TRUE(node->hasAttributes());
  Array<std::string>* attributes = node->getAttributes(nullptr);
  for (size_t i = 0; i < attributes->length() - 1; i++) {
    if (attributes->get(i) == attribute)
      return attributes->get(i + 1);
  }
  return nullptr;
}

bool Equals(aura::Window* window, DOM::Node* node) {
  int children_count = static_cast<int>(window->children().size());
  if (views::Widget::GetWidgetForNativeView(window))
    children_count++;
  return "Window" == node->getNodeName() &&
         window->GetName() == GetAttributeValue("name", node) &&
         children_count == node->getChildNodeCount(kDefaultChildNodeCount);
}

void Compare(views::Widget* widget, DOM::Node* node) {
  EXPECT_EQ("Widget", node->getNodeName());
  EXPECT_EQ(widget->GetName(), GetAttributeValue("name", node));
  EXPECT_EQ(widget->GetRootView() ? 1 : 0,
            node->getChildNodeCount(kDefaultChildNodeCount));
}

void Compare(views::View* view, DOM::Node* node) {
  EXPECT_EQ("View", node->getNodeName());
  EXPECT_EQ(view->GetClassName(), GetAttributeValue("name", node));
  EXPECT_EQ(view->child_count(),
            node->getChildNodeCount(kDefaultChildNodeCount));
}

void Compare(aura::Window* window, DOM::Node* node) {
  EXPECT_TRUE(Equals(window, node));
}

DOM::Node* FindInRoot(aura::Window* window, DOM::Node* root) {
  if (Equals(window, root))
    return root;

  Array<DOM::Node>* children = root->getChildren(nullptr);
  DOM::Node* window_node = nullptr;
  for (size_t i = 0; i < children->length(); i++) {
    window_node = FindInRoot(window, children->get(i));
    if (window_node)
      return window_node;
  }
  return window_node;
}

int GetIntPropertyByName(const std::string& name,
                         Array<CSS::CSSProperty>* properties) {
  for (size_t i = 0; i < properties->length(); i++) {
    CSS::CSSProperty* property = properties->get(i);
    if (property->getName() == name) {
      int value;
      EXPECT_TRUE(base::StringToInt(property->getValue(), &value));
      return value;
    }
  }
  NOTREACHED();
  return -1;
}

std::string GetStringPropertyByName(const std::string& name,
                                    Array<CSS::CSSProperty>* properties) {
  for (size_t i = 0; i < properties->length(); i++) {
    CSS::CSSProperty* property = properties->get(i);
    if (property->getName() == name)
      return property->getValue();
  }
  NOTREACHED();
  return nullptr;
}

ui::Layer* GetHighlightingLayer(aura::Window* root_window) {
  for (auto* layer : root_window->layer()->children()) {
    if (layer->name() == "HighlightingLayer")
      return layer;
  }
  return nullptr;
}

std::unique_ptr<DOM::RGBA> SkColorToRGBA(const SkColor& color) {
  return DOM::RGBA::create()
      .setA(SkColorGetA(color) / 255)
      .setB(SkColorGetB(color))
      .setG(SkColorGetG(color))
      .setR(SkColorGetR(color))
      .build();
}

std::unique_ptr<Overlay::HighlightConfig> CreateHighlightConfig(
    const SkColor& background_color,
    const SkColor& border_color) {
  return Overlay::HighlightConfig::create()
      .setContentColor(SkColorToRGBA(background_color))
      .setBorderColor(SkColorToRGBA(border_color))
      .build();
}

}  // namespace

class UIDevToolsTest : public views::ViewsTestBase {
 public:
  UIDevToolsTest() {}
  ~UIDevToolsTest() override {}

  views::internal::NativeWidgetPrivate* CreateTestNativeWidget() {
    views::Widget* widget = new views::Widget;
    views::Widget::InitParams params;
    params.ownership = views::Widget::InitParams::NATIVE_WIDGET_OWNS_WIDGET;
    params.parent = GetPrimaryRootWindow();
    widget->Init(params);
    return widget->native_widget_private();
  }

  std::unique_ptr<views::Widget> CreateTestWidget(const gfx::Rect& bounds) {
    std::unique_ptr<views::Widget> widget(new views::Widget);
    views::Widget::InitParams params;
    params.delegate = nullptr;
    params.ownership = views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
    params.bounds = bounds;
    params.parent = GetPrimaryRootWindow();
    widget->Init(params);
    widget->Show();
    return widget;
  }

  std::unique_ptr<aura::Window> CreateChildWindow(
      aura::Window* parent,
      aura::client::WindowType type = aura::client::WINDOW_TYPE_NORMAL) {
    std::unique_ptr<aura::Window> window =
        std::make_unique<aura::Window>(nullptr, type);
    window->Init(ui::LAYER_NOT_DRAWN);
    window->SetBounds(gfx::Rect());
    parent->AddChild(window.get());
    window->Show();
    return window;
  }

  void SetUp() override {
    fake_frontend_channel_ = std::make_unique<FakeFrontendChannel>();
    uber_dispatcher_ =
        std::make_unique<UberDispatcher>(fake_frontend_channel_.get());
    dom_agent_ = std::make_unique<DOMAgentAura>();
    dom_agent_->Init(uber_dispatcher_.get());
    css_agent_ = std::make_unique<CSSAgent>(dom_agent_.get());
    css_agent_->Init(uber_dispatcher_.get());
    css_agent_->enable();
    overlay_agent_ = std::make_unique<OverlayAgentAura>(dom_agent_.get());
    overlay_agent_->Init(uber_dispatcher_.get());
    overlay_agent_->enable();

    // We need to create |dom_agent| first to observe creation of
    // WindowTreeHosts in ViewTestBase::SetUp().
    views::ViewsTestBase::SetUp();

    top_window = CreateChildWindow(GetPrimaryRootWindow());
    top_default_container_window = CreateChildWindow(GetPrimaryRootWindow());
    top_overlay_window = CreateChildWindow(GetPrimaryRootWindow(),
                                           aura::client::WINDOW_TYPE_UNKNOWN);
  }

  void TearDown() override {
    top_overlay_window.reset();
    top_default_container_window.reset();
    top_window.reset();
    css_agent_.reset();
    overlay_agent_.reset();
    dom_agent_.reset();
    uber_dispatcher_.reset();
    fake_frontend_channel_.reset();
    views::ViewsTestBase::TearDown();
  }

  void ExpectChildNodeInserted(int parent_id, int prev_sibling_id) {
    EXPECT_EQ(1, frontend_channel()->CountProtocolNotificationMessageStartsWith(
                     base::StringPrintf("{\"method\":\"DOM.childNodeInserted\","
                                        "\"params\":{\"parentNodeId\":%d,"
                                        "\"previousNodeId\":%d",
                                        parent_id, prev_sibling_id)));
  }

  void ExpectChildNodeRemoved(int parent_id, int node_id) {
    EXPECT_EQ(1, frontend_channel()->CountProtocolNotificationMessage(
                     base::StringPrintf(
                         "{\"method\":\"DOM.childNodeRemoved\",\"params\":{"
                         "\"parentNodeId\":%d,\"nodeId\":%d}}",
                         parent_id, node_id)));
  }

  int GetStyleSheetChangedCount(int node_id) {
    return frontend_channel()->CountProtocolNotificationMessage(
        base::StringPrintf("{\"method\":\"CSS.styleSheetChanged\",\"params\":{"
                           "\"styleSheetId\":\"%d\"}}",
                           node_id));
  }

  int GetOverlayNodeHighlightRequestedCount(int node_id) {
    return frontend_channel()->CountProtocolNotificationMessage(
        base::StringPrintf(
            "{\"method\":\"Overlay.nodeHighlightRequested\",\"params\":{"
            "\"nodeId\":%d}}",
            node_id));
  }

  int GetOverlayInspectNodeRequestedCount(int node_id) {
    return frontend_channel()->CountProtocolNotificationMessage(
        base::StringPrintf(
            "{\"method\":\"Overlay.inspectNodeRequested\",\"params\":{"
            "\"backendNodeId\":%d}}",
            node_id));
  }

  void CompareNodeBounds(DOM::Node* node, const gfx::Rect& bounds) {
    Maybe<CSS::CSSStyle> styles;
    css_agent_->getMatchedStylesForNode(node->getNodeId(), &styles);
    ASSERT_TRUE(styles.isJust());
    Array<CSS::CSSProperty>* properties = styles.fromJust()->getCssProperties();
    EXPECT_EQ(bounds.height(), GetIntPropertyByName("height", properties));
    EXPECT_EQ(bounds.width(), GetIntPropertyByName("width", properties));
    EXPECT_EQ(bounds.x(), GetIntPropertyByName("x", properties));
    EXPECT_EQ(bounds.y(), GetIntPropertyByName("y", properties));
  }

  void CompareViewAtrributes(DOM::Node* node, views::View* view) {
    Maybe<CSS::CSSStyle> styles;
    css_agent_->getMatchedStylesForNode(node->getNodeId(), &styles);
    ASSERT_TRUE(styles.isJust());
    Array<CSS::CSSProperty>* properties = styles.fromJust()->getCssProperties();

    base::string16 description;
    if (view->GetTooltipText(gfx::Point(), &description)) {
      EXPECT_EQ(base::UTF16ToUTF8(description),
                GetStringPropertyByName("tooltip", properties));
    }
  }

  void SetStyleTexts(DOM::Node* node,
                     const std::string& style_text,
                     bool success) {
    auto edits = Array<CSS::StyleDeclarationEdit>::create();
    auto edit = CSS::StyleDeclarationEdit::create()
                    .setStyleSheetId(base::IntToString(node->getNodeId()))
                    .setText(style_text)
                    .build();
    edits->addItem(std::move(edit));
    std::unique_ptr<Array<CSS::CSSStyle>> output;
    EXPECT_EQ(success,
              css_agent_->setStyleTexts(std::move(edits), &output).isSuccess());

    if (success)
      ASSERT_TRUE(output);
    else
      ASSERT_FALSE(output);
  }

  void HighlightNode(int node_id) {
    overlay_agent_->highlightNode(
        CreateHighlightConfig(kBackgroundColor, kBorderColor), node_id);
  }

  void HideHighlight(int root_window_index) {
    overlay_agent_->hideHighlight();
    DCHECK_GE(root_window_index, 0);
    DCHECK_LE(root_window_index,
              static_cast<int>(dom_agent()->root_windows().size()));
    ASSERT_FALSE(
        GetHighlightingLayer(dom_agent()->root_windows()[root_window_index])
            ->visible());
  }

  FakeFrontendChannel* frontend_channel() {
    return fake_frontend_channel_.get();
  }

  aura::Window* GetPrimaryRootWindow() {
    DCHECK(dom_agent()->root_windows().size());
    return dom_agent()->root_windows()[0];
  }

  CSSAgent* css_agent() { return css_agent_.get(); }
  DOMAgentAura* dom_agent() { return dom_agent_.get(); }
  OverlayAgentAura* overlay_agent() { return overlay_agent_.get(); }

  std::unique_ptr<aura::Window> top_overlay_window;
  std::unique_ptr<aura::Window> top_window;
  std::unique_ptr<aura::Window> top_default_container_window;

 private:
  std::unique_ptr<UberDispatcher> uber_dispatcher_;
  std::unique_ptr<FakeFrontendChannel> fake_frontend_channel_;
  std::unique_ptr<DOMAgentAura> dom_agent_;
  std::unique_ptr<CSSAgent> css_agent_;
  std::unique_ptr<OverlayAgentAura> overlay_agent_;

  DISALLOW_COPY_AND_ASSIGN(UIDevToolsTest);
};

// Tests that FindElementIdTargetedByPoint() returns a non-zero id when a UI
// element target exists.
TEST_F(UIDevToolsTest, FindElementIdTargetedByPoint) {
  std::unique_ptr<views::Widget> widget(
      CreateTestWidget(gfx::Rect(1, 1, 1, 1)));
  std::unique_ptr<DOM::Node> root;
  dom_agent()->getDocument(&root);
  EXPECT_NE(0, overlay_agent()->FindElementIdTargetedByPoint(
                   gfx::Point(1, 1), GetPrimaryRootWindow()));
}

// Test case R1_CONTAINS_R2.
TEST_F(UIDevToolsTest, OneUIElementContainsAnother) {
  const gfx::Rect outside_rect(1, 1, 100, 100);
  std::unique_ptr<views::Widget> widget_outside(CreateTestWidget(outside_rect));

  const gfx::Rect inside_rect(2, 2, 50, 50);
  std::unique_ptr<views::Widget> widget_inside(CreateTestWidget(inside_rect));

  std::unique_ptr<DOM::Node> root;
  dom_agent()->getDocument(&root);

  int outside_rect_id = overlay_agent()->FindElementIdTargetedByPoint(
      outside_rect.origin(), GetPrimaryRootWindow());
  int inside_rect_id = overlay_agent()->FindElementIdTargetedByPoint(
      inside_rect.origin(), GetPrimaryRootWindow());
  overlay_agent()->ShowDistancesInHighlightOverlay(outside_rect_id,
                                                   inside_rect_id);

  HighlightRectsConfiguration highlight_rect_config =
      overlay_agent()->highlight_rect_config();

  // Swapping R1 and R2 shouldn't change |highlight_rect_config|.
  overlay_agent()->ShowDistancesInHighlightOverlay(inside_rect_id,
                                                   outside_rect_id);
  DCHECK_EQ(highlight_rect_config, overlay_agent()->highlight_rect_config());

  const std::pair<aura::Window*, gfx::Rect> element_outside(
      dom_agent()
          ->GetElementFromNodeId(outside_rect_id)
          ->GetNodeWindowAndBounds());

  const std::pair<aura::Window*, gfx::Rect> element_inside(
      dom_agent()
          ->GetElementFromNodeId(inside_rect_id)
          ->GetNodeWindowAndBounds());

  EXPECT_EQ(element_outside.second, outside_rect);
  EXPECT_EQ(element_inside.second, inside_rect);
  DCHECK_EQ(overlay_agent()->highlight_rect_config(),
            HighlightRectsConfiguration::R1_CONTAINS_R2);
}

// Test case R1_HORIZONTAL_FULL_LEFT_R2.
TEST_F(UIDevToolsTest, OneUIElementStaysHorizontalAndLeftOfAnother) {
  const gfx::Rect outside_rect(1, 1, 50, 50);
  std::unique_ptr<views::Widget> widget_outside(CreateTestWidget(outside_rect));

  const gfx::Rect inside_rect(60, 1, 60, 60);
  std::unique_ptr<views::Widget> widget_inside(CreateTestWidget(inside_rect));

  std::unique_ptr<DOM::Node> root;
  dom_agent()->getDocument(&root);

  int outside_rect_id = overlay_agent()->FindElementIdTargetedByPoint(
      outside_rect.origin(), GetPrimaryRootWindow());
  int inside_rect_id = overlay_agent()->FindElementIdTargetedByPoint(
      inside_rect.origin(), GetPrimaryRootWindow());
  overlay_agent()->ShowDistancesInHighlightOverlay(outside_rect_id,
                                                   inside_rect_id);

  HighlightRectsConfiguration highlight_rect_config =
      overlay_agent()->highlight_rect_config();

  // Swapping R1 and R2 shouldn't change |highlight_rect_config|.
  overlay_agent()->ShowDistancesInHighlightOverlay(inside_rect_id,
                                                   outside_rect_id);
  DCHECK_EQ(highlight_rect_config, overlay_agent()->highlight_rect_config());

  const std::pair<aura::Window*, gfx::Rect> element_outside(
      dom_agent()
          ->GetElementFromNodeId(outside_rect_id)
          ->GetNodeWindowAndBounds());

  const std::pair<aura::Window*, gfx::Rect> element_inside(
      dom_agent()
          ->GetElementFromNodeId(inside_rect_id)
          ->GetNodeWindowAndBounds());

  EXPECT_EQ(element_outside.second, outside_rect);
  EXPECT_EQ(element_inside.second, inside_rect);
  DCHECK_EQ(overlay_agent()->highlight_rect_config(),
            HighlightRectsConfiguration::R1_HORIZONTAL_FULL_LEFT_R2);
}

// Test case R1_TOP_FULL_LEFT_R2.
TEST_F(UIDevToolsTest, OneUIElementStaysFullyTopLeftOfAnother) {
  const gfx::Rect top_left_rect(30, 30, 50, 50);
  std::unique_ptr<views::Widget> widget_top_left(
      CreateTestWidget(top_left_rect));

  const gfx::Rect bottom_right_rect(100, 100, 50, 50);
  std::unique_ptr<views::Widget> widget_bottom_right(
      CreateTestWidget(bottom_right_rect));

  std::unique_ptr<DOM::Node> root;
  dom_agent()->getDocument(&root);

  int top_left_rect_id = overlay_agent()->FindElementIdTargetedByPoint(
      top_left_rect.origin(), GetPrimaryRootWindow());
  int bottom_right_rect_id = overlay_agent()->FindElementIdTargetedByPoint(
      bottom_right_rect.origin(), GetPrimaryRootWindow());
  overlay_agent()->ShowDistancesInHighlightOverlay(top_left_rect_id,
                                                   bottom_right_rect_id);

  HighlightRectsConfiguration highlight_rect_config =
      overlay_agent()->highlight_rect_config();

  // Swapping R1 and R2 shouldn't change |highlight_rect_config|.
  overlay_agent()->ShowDistancesInHighlightOverlay(bottom_right_rect_id,
                                                   top_left_rect_id);
  DCHECK_EQ(highlight_rect_config, overlay_agent()->highlight_rect_config());

  const std::pair<aura::Window*, gfx::Rect> element_top_left(
      dom_agent()
          ->GetElementFromNodeId(top_left_rect_id)
          ->GetNodeWindowAndBounds());

  const std::pair<aura::Window*, gfx::Rect> element_bottom_right(
      dom_agent()
          ->GetElementFromNodeId(bottom_right_rect_id)
          ->GetNodeWindowAndBounds());

  EXPECT_EQ(element_top_left.second, top_left_rect);
  EXPECT_EQ(element_bottom_right.second, bottom_right_rect);
  DCHECK_EQ(overlay_agent()->highlight_rect_config(),
            HighlightRectsConfiguration::R1_TOP_FULL_LEFT_R2);
}

// Test case R1_BOTTOM_FULL_LEFT_R2.
TEST_F(UIDevToolsTest, OneUIElementStaysFullyBottomLeftOfAnother) {
  const gfx::Rect bottom_left_rect(100, 100, 50, 50);
  std::unique_ptr<views::Widget> widget_bottom_left(
      CreateTestWidget(bottom_left_rect));

  const gfx::Rect top_right_rect(200, 50, 40, 40);
  std::unique_ptr<views::Widget> widget_top_right(
      CreateTestWidget(top_right_rect));

  std::unique_ptr<DOM::Node> root;
  dom_agent()->getDocument(&root);

  int bottom_left_rect_id = overlay_agent()->FindElementIdTargetedByPoint(
      bottom_left_rect.origin(), GetPrimaryRootWindow());
  int top_right_rect_id = overlay_agent()->FindElementIdTargetedByPoint(
      top_right_rect.origin(), GetPrimaryRootWindow());
  overlay_agent()->ShowDistancesInHighlightOverlay(bottom_left_rect_id,
                                                   top_right_rect_id);

  HighlightRectsConfiguration highlight_rect_config =
      overlay_agent()->highlight_rect_config();

  // Swapping R1 and R2 shouldn't change |highlight_rect_config|.
  overlay_agent()->ShowDistancesInHighlightOverlay(top_right_rect_id,
                                                   bottom_left_rect_id);

  DCHECK_EQ(highlight_rect_config, overlay_agent()->highlight_rect_config());

  const std::pair<aura::Window*, gfx::Rect> element_top_left(
      dom_agent()
          ->GetElementFromNodeId(bottom_left_rect_id)
          ->GetNodeWindowAndBounds());

  const std::pair<aura::Window*, gfx::Rect> element_bottom_right(
      dom_agent()
          ->GetElementFromNodeId(top_right_rect_id)
          ->GetNodeWindowAndBounds());

  EXPECT_EQ(element_top_left.second, bottom_left_rect);
  EXPECT_EQ(element_bottom_right.second, top_right_rect);
  DCHECK_EQ(overlay_agent()->highlight_rect_config(),
            HighlightRectsConfiguration::R1_BOTTOM_FULL_LEFT_R2);
}

// Test case R1_TOP_PARTIAL_LEFT_R2.
TEST_F(UIDevToolsTest, OneUIElementStaysPartiallyTopLeftOfAnother) {
  const gfx::Rect top_left_rect(100, 100, 50, 50);
  std::unique_ptr<views::Widget> widget_top_left(
      CreateTestWidget(top_left_rect));

  const gfx::Rect bottom_right_rect(120, 200, 50, 50);
  std::unique_ptr<views::Widget> widget_bottom_right(
      CreateTestWidget(bottom_right_rect));

  std::unique_ptr<DOM::Node> root;
  dom_agent()->getDocument(&root);

  int top_left_rect_id = overlay_agent()->FindElementIdTargetedByPoint(
      top_left_rect.origin(), GetPrimaryRootWindow());
  int bottom_right_rect_id = overlay_agent()->FindElementIdTargetedByPoint(
      bottom_right_rect.origin(), GetPrimaryRootWindow());
  overlay_agent()->ShowDistancesInHighlightOverlay(top_left_rect_id,
                                                   bottom_right_rect_id);

  HighlightRectsConfiguration highlight_rect_config =
      overlay_agent()->highlight_rect_config();

  // Swapping R1 and R2 shouldn't change |highlight_rect_config|.
  overlay_agent()->ShowDistancesInHighlightOverlay(bottom_right_rect_id,
                                                   top_left_rect_id);
  DCHECK_EQ(highlight_rect_config, overlay_agent()->highlight_rect_config());

  const std::pair<aura::Window*, gfx::Rect> element_top_left(
      dom_agent()
          ->GetElementFromNodeId(top_left_rect_id)
          ->GetNodeWindowAndBounds());

  const std::pair<aura::Window*, gfx::Rect> element_bottom_right(
      dom_agent()
          ->GetElementFromNodeId(bottom_right_rect_id)
          ->GetNodeWindowAndBounds());

  EXPECT_EQ(element_top_left.second, top_left_rect);
  EXPECT_EQ(element_bottom_right.second, bottom_right_rect);
  DCHECK_EQ(overlay_agent()->highlight_rect_config(),
            HighlightRectsConfiguration::R1_TOP_PARTIAL_LEFT_R2);
}

// Test case R1_BOTTOM_PARTIAL_LEFT_R2.
TEST_F(UIDevToolsTest, OneUIElementStaysPartiallyBottomLeftOfAnother) {
  const gfx::Rect bottom_left_rect(50, 200, 100, 100);
  std::unique_ptr<views::Widget> widget_bottom_left(
      CreateTestWidget(bottom_left_rect));

  const gfx::Rect top_right_rect(100, 50, 50, 50);
  std::unique_ptr<views::Widget> widget_top_right(
      CreateTestWidget(top_right_rect));

  std::unique_ptr<DOM::Node> root;
  dom_agent()->getDocument(&root);

  int bottom_left_rect_id = overlay_agent()->FindElementIdTargetedByPoint(
      bottom_left_rect.origin(), GetPrimaryRootWindow());
  int top_right_rect_id = overlay_agent()->FindElementIdTargetedByPoint(
      top_right_rect.origin(), GetPrimaryRootWindow());
  overlay_agent()->ShowDistancesInHighlightOverlay(bottom_left_rect_id,
                                                   top_right_rect_id);

  HighlightRectsConfiguration highlight_rect_config =
      overlay_agent()->highlight_rect_config();

  // Swapping R1 and R2 shouldn't change |highlight_rect_config|.
  overlay_agent()->ShowDistancesInHighlightOverlay(top_right_rect_id,
                                                   bottom_left_rect_id);
  DCHECK_EQ(highlight_rect_config, overlay_agent()->highlight_rect_config());

  const std::pair<aura::Window*, gfx::Rect> element_bottom_left(
      dom_agent()
          ->GetElementFromNodeId(bottom_left_rect_id)
          ->GetNodeWindowAndBounds());

  const std::pair<aura::Window*, gfx::Rect> element_top_right(
      dom_agent()
          ->GetElementFromNodeId(top_right_rect_id)
          ->GetNodeWindowAndBounds());

  EXPECT_EQ(element_bottom_left.second, bottom_left_rect);
  EXPECT_EQ(element_top_right.second, top_right_rect);
  DCHECK_EQ(overlay_agent()->highlight_rect_config(),
            HighlightRectsConfiguration::R1_BOTTOM_PARTIAL_LEFT_R2);
}

// Test case R1_INTERSECTS_R2.
TEST_F(UIDevToolsTest, OneUIElementIntersectsAnother) {
  const gfx::Rect left_rect(100, 100, 50, 50);
  std::unique_ptr<views::Widget> widget_left(CreateTestWidget(left_rect));

  const gfx::Rect right_rect(120, 120, 50, 50);
  std::unique_ptr<views::Widget> widget_right(CreateTestWidget(right_rect));

  std::unique_ptr<DOM::Node> root;
  dom_agent()->getDocument(&root);

  int left_rect_id = overlay_agent()->FindElementIdTargetedByPoint(
      left_rect.origin(), GetPrimaryRootWindow());
  int right_rect_id = overlay_agent()->FindElementIdTargetedByPoint(
      right_rect.origin(), GetPrimaryRootWindow());
  overlay_agent()->ShowDistancesInHighlightOverlay(left_rect_id, right_rect_id);

  HighlightRectsConfiguration highlight_rect_config =
      overlay_agent()->highlight_rect_config();

  // Swapping R1 and R2 shouldn't change |highlight_rect_config|.
  overlay_agent()->ShowDistancesInHighlightOverlay(right_rect_id, left_rect_id);
  DCHECK_EQ(highlight_rect_config, overlay_agent()->highlight_rect_config());

  const std::pair<aura::Window*, gfx::Rect> element_left(
      dom_agent()
          ->GetElementFromNodeId(left_rect_id)
          ->GetNodeWindowAndBounds());

  const std::pair<aura::Window*, gfx::Rect> element_right(
      dom_agent()
          ->GetElementFromNodeId(right_rect_id)
          ->GetNodeWindowAndBounds());

  EXPECT_EQ(element_left.second, left_rect);
  EXPECT_EQ(element_right.second, right_rect);
  DCHECK_EQ(overlay_agent()->highlight_rect_config(),
            HighlightRectsConfiguration::R1_INTERSECTS_R2);
}

// Tests that the correct Overlay events are dispatched to the frontend when
// hovering and clicking over a UI element in inspect mode.
TEST_F(UIDevToolsTest, MouseEventsGenerateFEEventsInInspectMode) {
  std::unique_ptr<views::Widget> widget(
      CreateTestWidget(gfx::Rect(1, 1, 1, 1)));

  std::unique_ptr<DOM::Node> root;
  dom_agent()->getDocument(&root);

  gfx::Point p(1, 1);
  int node_id =
      overlay_agent()->FindElementIdTargetedByPoint(p, GetPrimaryRootWindow());

  EXPECT_EQ(0, GetOverlayInspectNodeRequestedCount(node_id));
  EXPECT_EQ(0, GetOverlayNodeHighlightRequestedCount(node_id));
  overlay_agent()->setInspectMode(
      "searchForNode", protocol::Maybe<protocol::Overlay::HighlightConfig>());

  // Moving the mouse cursor over the widget bounds should request a node
  // highlight.
  ui::test::EventGenerator generator(widget->GetNativeWindow());
  generator.MoveMouseBy(p.x(), p.y());

  // 2 mouse events ET_MOUSE_ENTERED and ET_MOUSE_MOVED are generated.
  EXPECT_EQ(2, GetOverlayNodeHighlightRequestedCount(node_id));
  EXPECT_EQ(0, GetOverlayInspectNodeRequestedCount(node_id));

  // Clicking on the widget should pin that element.
  generator.PressLeftButton();

  // Pin parent node after mouse wheel moves up.
  int parent_id = dom_agent()->GetParentIdOfNodeId(node_id);
  EXPECT_NE(parent_id, overlay_agent()->pinned_id());
  generator.MoveMouseWheel(0, 1);
  EXPECT_EQ(parent_id, overlay_agent()->pinned_id());

  // Re-assign pin node.
  node_id = parent_id;

  int inspect_node_notification_count =
      GetOverlayInspectNodeRequestedCount(node_id);

  // Press escape to exit inspect mode.
  generator.PressKey(ui::KeyboardCode::VKEY_ESCAPE, ui::EventFlags::EF_NONE);

  // Upon exiting inspect mode, the element is inspected and highlighted.
  EXPECT_EQ(inspect_node_notification_count + 1,
            GetOverlayInspectNodeRequestedCount(node_id));
  ui::Layer* highlighting_layer = GetHighlightingLayer(GetPrimaryRootWindow());
  EXPECT_EQ(kBackgroundColor, highlighting_layer->GetTargetColor());
  EXPECT_TRUE(highlighting_layer->visible());

  int highlight_notification_count =
      GetOverlayNodeHighlightRequestedCount(node_id);
  inspect_node_notification_count =
      GetOverlayInspectNodeRequestedCount(node_id);

  // Since inspect mode is exited, a subsequent mouse move should generate no
  // nodeHighlightRequested or inspectNodeRequested events.
  generator.MoveMouseBy(p.x(), p.y());
  EXPECT_EQ(highlight_notification_count,
            GetOverlayNodeHighlightRequestedCount(node_id));
  EXPECT_EQ(inspect_node_notification_count,
            GetOverlayInspectNodeRequestedCount(node_id));
}

TEST_F(UIDevToolsTest, GetDocumentWithWindowWidgetView) {
  std::unique_ptr<views::Widget> widget(
      CreateTestWidget(gfx::Rect(1, 1, 1, 1)));
  aura::Window* parent_window = widget->GetNativeWindow();
  parent_window->SetName("parent_window");
  std::unique_ptr<aura::Window> child_window = CreateChildWindow(parent_window);
  child_window->SetName("child_window");
  widget->Show();
  views::View* child_view = new TestView("child_view");
  widget->GetRootView()->AddChildView(child_view);

  std::unique_ptr<DOM::Node> root;
  dom_agent()->getDocument(&root);

  DOM::Node* parent_node = FindInRoot(parent_window, root.get());
  ASSERT_TRUE(parent_node);
  Array<DOM::Node>* parent_children = parent_node->getChildren(nullptr);
  ASSERT_TRUE(parent_children);
  DOM::Node* widget_node = parent_children->get(0);
  Compare(widget.get(), widget_node);
  Compare(child_window.get(), parent_children->get(1));
  Array<DOM::Node>* widget_children = widget_node->getChildren(nullptr);
  ASSERT_TRUE(widget_children);
  Compare(widget->GetRootView(), widget_children->get(0));
  ASSERT_TRUE(widget_children->get(0)->getChildren(nullptr));
  Compare(child_view, widget_children->get(0)->getChildren(nullptr)->get(1));
}

TEST_F(UIDevToolsTest, GetDocumentNativeWidgetOwnsWidget) {
  views::internal::NativeWidgetPrivate* native_widget_private =
      CreateTestNativeWidget();
  views::Widget* widget = native_widget_private->GetWidget();
  aura::Window* parent_window = widget->GetNativeWindow();

  std::unique_ptr<DOM::Node> root;
  dom_agent()->getDocument(&root);

  DOM::Node* parent_node = FindInRoot(parent_window, root.get());
  ASSERT_TRUE(parent_node);
  DOM::Node* widget_node = parent_node->getChildren(nullptr)->get(0);
  Compare(widget, widget_node);
  // Destroy NativeWidget followed by |widget|
  widget->CloseNow();
}

TEST_F(UIDevToolsTest, WindowAddedChildNodeInserted) {
  // Initialize DOMAgent
  std::unique_ptr<aura::Window> window_child =
      CreateChildWindow(top_window.get());

  std::unique_ptr<DOM::Node> root;
  dom_agent()->getDocument(&root);

  aura::Window* root_window = GetPrimaryRootWindow();
  aura::Window* parent_window = root_window->children()[0];
  DOM::Node* parent_node = FindInRoot(parent_window, root.get());
  Array<DOM::Node>* parent_node_children = parent_node->getChildren(nullptr);
  DOM::Node* sibling_node =
      parent_node_children->get(parent_node_children->length() - 1);

  std::unique_ptr<aura::Window> child(CreateChildWindow(parent_window));
  ExpectChildNodeInserted(parent_node->getNodeId(), sibling_node->getNodeId());
}

TEST_F(UIDevToolsTest, WindowDestroyedChildNodeRemoved) {
  std::unique_ptr<aura::Window> child_1 = CreateChildWindow(top_window.get());
  std::unique_ptr<aura::Window> child_2 = CreateChildWindow(child_1.get());

  // Initialize DOMAgent
  std::unique_ptr<DOM::Node> root;
  dom_agent()->getDocument(&root);

  aura::Window* root_window = GetPrimaryRootWindow();
  aura::Window* rotation_window = root_window->children()[0];
  aura::Window* parent_window = rotation_window->children()[0];
  aura::Window* child_window = parent_window->children()[0];
  DOM::Node* root_node =
      root->getChildren(nullptr)->get(0)->getChildren(nullptr)->get(0);
  DOM::Node* parent_node = root_node->getChildren(nullptr)->get(0);
  DOM::Node* child_node = parent_node->getChildren(nullptr)->get(0);

  Compare(parent_window, parent_node);
  Compare(child_window, child_node);
  child_2.reset();
  ExpectChildNodeRemoved(parent_node->getNodeId(), child_node->getNodeId());
}

TEST_F(UIDevToolsTest, WindowReorganizedChildNodeRearranged) {
  std::unique_ptr<aura::Window> child_1 = CreateChildWindow(top_window.get());
  std::unique_ptr<aura::Window> child_2 = CreateChildWindow(top_window.get());
  std::unique_ptr<aura::Window> child_11 = CreateChildWindow(child_1.get());
  std::unique_ptr<aura::Window> child_21 = CreateChildWindow(child_2.get());

  // Initialize DOMAgent
  std::unique_ptr<DOM::Node> root;
  dom_agent()->getDocument(&root);

  aura::Window* root_window = GetPrimaryRootWindow();
  aura::Window* rotation_window = root_window->children()[0];
  aura::Window* parent_window = rotation_window->children()[0];
  aura::Window* target_window = rotation_window->children()[1];
  aura::Window* child_window = parent_window->children()[0];

  DOM::Node* root_node =
      root->getChildren(nullptr)->get(0)->getChildren(nullptr)->get(0);
  DOM::Node* parent_node = root_node->getChildren(nullptr)->get(0);
  DOM::Node* target_node = root_node->getChildren(nullptr)->get(1);
  Array<DOM::Node>* target_node_children = target_node->getChildren(nullptr);
  DOM::Node* sibling_node =
      target_node_children->get(target_node_children->length() - 1);
  DOM::Node* child_node = parent_node->getChildren(nullptr)->get(0);

  Compare(parent_window, parent_node);
  Compare(target_window, target_node);
  Compare(child_window, child_node);
  target_window->AddChild(child_window);
  ExpectChildNodeRemoved(parent_node->getNodeId(), child_node->getNodeId());
  ExpectChildNodeInserted(target_node->getNodeId(), sibling_node->getNodeId());
}

TEST_F(UIDevToolsTest, WindowReorganizedChildNodeRemovedAndInserted) {
  std::unique_ptr<aura::Window> child_1 = CreateChildWindow(top_window.get());
  std::unique_ptr<aura::Window> child_2 = CreateChildWindow(top_window.get());
  std::unique_ptr<aura::Window> child_21 = CreateChildWindow(child_2.get());
  std::unique_ptr<aura::Window> child_22 = CreateChildWindow(child_2.get());

  aura::Window* root_window = GetPrimaryRootWindow();
  aura::Window* rotation_window = root_window->children()[0];
  aura::Window* parent_window = rotation_window->children()[0];
  aura::Window* target_window = rotation_window->children()[1];
  std::unique_ptr<aura::Window> child_window(CreateChildWindow(parent_window));

  // Initialize DOMAgent
  std::unique_ptr<DOM::Node> root;
  dom_agent()->getDocument(&root);
  DOM::Node* root_node =
      root->getChildren(nullptr)->get(0)->getChildren(nullptr)->get(0);
  DOM::Node* parent_node = root_node->getChildren(nullptr)->get(0);
  DOM::Node* target_node = root_node->getChildren(nullptr)->get(1);
  Array<DOM::Node>* target_node_children = target_node->getChildren(nullptr);
  DOM::Node* sibling_node =
      target_node_children->get(target_node_children->length() - 1);
  Array<DOM::Node>* parent_node_children = parent_node->getChildren(nullptr);
  DOM::Node* child_node =
      parent_node_children->get(parent_node_children->length() - 1);

  Compare(parent_window, parent_node);
  Compare(target_window, target_node);
  Compare(child_window.get(), child_node);
  parent_window->RemoveChild(child_window.get());
  target_window->AddChild(child_window.get());
  ExpectChildNodeRemoved(parent_node->getNodeId(), child_node->getNodeId());
  ExpectChildNodeInserted(target_node->getNodeId(), sibling_node->getNodeId());
}

TEST_F(UIDevToolsTest, WindowStackingChangedChildNodeRemovedAndInserted) {
  std::unique_ptr<aura::Window> child_11 = CreateChildWindow(top_window.get());
  std::unique_ptr<aura::Window> child_12 = CreateChildWindow(top_window.get());
  std::unique_ptr<aura::Window> child_13 = CreateChildWindow(top_window.get());

  // Initialize DOMAgent
  std::unique_ptr<DOM::Node> root;
  dom_agent()->getDocument(&root);

  aura::Window* root_window = GetPrimaryRootWindow();
  aura::Window* parent_window = root_window->children()[0];
  aura::Window* child_window = parent_window->children()[0];
  aura::Window* target_window = parent_window->children()[1];

  DOM::Node* parent_node =
      root->getChildren(nullptr)->get(0)->getChildren(nullptr)->get(0);
  Array<DOM::Node>* parent_node_children = parent_node->getChildren(nullptr);
  DOM::Node* child_node = parent_node_children->get(0);
  DOM::Node* sibling_node = parent_node_children->get(1);
  int parent_id = parent_node->getNodeId();

  Compare(parent_window, parent_node);
  Compare(child_window, child_node);
  parent_window->StackChildAbove(child_window, target_window);
  ExpectChildNodeRemoved(parent_id, child_node->getNodeId());
  ExpectChildNodeInserted(parent_id, sibling_node->getNodeId());
}

TEST_F(UIDevToolsTest, ViewInserted) {
  std::unique_ptr<views::Widget> widget(
      CreateTestWidget(gfx::Rect(1, 1, 1, 1)));
  aura::Window* window = widget->GetNativeWindow();
  widget->Show();

  // Initialize DOMAgent
  std::unique_ptr<DOM::Node> root;
  dom_agent()->getDocument(&root);

  DOM::Node* parent_node = FindInRoot(window, root.get());
  ASSERT_TRUE(parent_node);
  DOM::Node* widget_node = parent_node->getChildren(nullptr)->get(0);
  DOM::Node* root_view_node = widget_node->getChildren(nullptr)->get(0);
  Array<DOM::Node>* root_view_children = root_view_node->getChildren(nullptr);
  ASSERT_TRUE(root_view_children);
  DOM::Node* sibling_view_node =
      root_view_children->get(root_view_children->length() - 1);

  widget->GetRootView()->AddChildView(new views::View);
  ExpectChildNodeInserted(root_view_node->getNodeId(),
                          sibling_view_node->getNodeId());
}

TEST_F(UIDevToolsTest, ViewRemoved) {
  std::unique_ptr<views::Widget> widget(
      CreateTestWidget(gfx::Rect(1, 1, 1, 1)));
  // Need to store |view| in unique_ptr because it is removed from the widget
  // and needs to be destroyed independently
  std::unique_ptr<views::View> child_view = std::make_unique<views::View>();
  aura::Window* window = widget->GetNativeWindow();
  widget->Show();
  views::View* root_view = widget->GetRootView();
  root_view->AddChildView(child_view.get());

  // Initialize DOMAgent
  std::unique_ptr<DOM::Node> root;
  dom_agent()->getDocument(&root);

  DOM::Node* parent_node = FindInRoot(window, root.get());
  ASSERT_TRUE(parent_node);
  DOM::Node* widget_node = parent_node->getChildren(nullptr)->get(0);
  DOM::Node* root_view_node = widget_node->getChildren(nullptr)->get(0);
  Array<DOM::Node>* root_view_children = root_view_node->getChildren(nullptr);
  ASSERT_TRUE(root_view_children);
  DOM::Node* child_view_node =
      root_view_children->get(root_view_children->length() - 1);

  Compare(child_view.get(), child_view_node);
  root_view->RemoveChildView(child_view.get());
  ExpectChildNodeRemoved(root_view_node->getNodeId(),
                         child_view_node->getNodeId());
}

TEST_F(UIDevToolsTest, ViewRearranged) {
  std::unique_ptr<views::Widget> widget(
      CreateTestWidget(gfx::Rect(1, 1, 1, 1)));
  aura::Window* window = widget->GetNativeWindow();
  widget->Show();
  views::View* root_view = widget->GetRootView();
  views::View* parent_view = new views::View;
  views::View* target_view = new views::View;
  views::View* child_view = new views::View;
  views::View* child_view_1 = new views::View;

  root_view->AddChildView(parent_view);
  root_view->AddChildView(target_view);
  parent_view->AddChildView(child_view);
  parent_view->AddChildView(child_view_1);

  // Initialize DOMAgent
  std::unique_ptr<DOM::Node> root;
  dom_agent()->getDocument(&root);

  DOM::Node* parent_node = FindInRoot(window, root.get());
  ASSERT_TRUE(parent_node);
  DOM::Node* widget_node = parent_node->getChildren(nullptr)->get(0);
  DOM::Node* root_view_node = widget_node->getChildren(nullptr)->get(0);
  Array<DOM::Node>* root_view_children = root_view_node->getChildren(nullptr);
  ASSERT_TRUE(root_view_children);
  size_t root_children_size = root_view_children->length();
  ASSERT_TRUE(root_children_size >= 2);
  DOM::Node* parent_view_node = root_view_children->get(root_children_size - 2);
  DOM::Node* target_view_node = root_view_children->get(root_children_size - 1);
  DOM::Node* child_view_node = parent_view_node->getChildren(nullptr)->get(0);
  DOM::Node* child_view_node_1 = parent_view_node->getChildren(nullptr)->get(1);

  Compare(parent_view, parent_view_node);
  Compare(target_view, target_view_node);
  Compare(child_view, child_view_node);
  Compare(child_view_1, child_view_node_1);

  ASSERT_NE(child_view_node->getNodeId(), child_view_node_1->getNodeId());

  // Reorder child_view_1 from index 1 to 0 in view::Views tree. This makes DOM
  // tree remove view node at position 1 and insert it at position 0.
  parent_view->ReorderChildView(child_view_1, 0);
  ExpectChildNodeRemoved(parent_view_node->getNodeId(),
                         child_view_node_1->getNodeId());
  ExpectChildNodeInserted(parent_view_node->getNodeId(), 0);

  // Reorder child_view_1 to the same index 0 shouldn't perform reroder work, so
  // we still expect 1 remove and 1 insert protocol notification messages.
  parent_view->ReorderChildView(child_view_1, 0);
  ExpectChildNodeRemoved(parent_view_node->getNodeId(),
                         child_view_node_1->getNodeId());
  ExpectChildNodeInserted(parent_view_node->getNodeId(), 0);

  target_view->AddChildView(child_view);
  ExpectChildNodeRemoved(parent_view_node->getNodeId(),
                         child_view_node->getNodeId());
  ExpectChildNodeInserted(target_view_node->getNodeId(), 0);
}

TEST_F(UIDevToolsTest, ViewRearrangedRemovedAndInserted) {
  std::unique_ptr<views::Widget> widget(
      CreateTestWidget(gfx::Rect(1, 1, 1, 1)));
  aura::Window* window = widget->GetNativeWindow();
  widget->Show();
  views::View* root_view = widget->GetRootView();
  views::View* parent_view = new views::View;
  views::View* target_view = new views::View;
  views::View* child_view = new views::View;
  root_view->AddChildView(parent_view);
  root_view->AddChildView(target_view);
  parent_view->AddChildView(child_view);

  // Initialize DOMAgent
  std::unique_ptr<DOM::Node> root;
  dom_agent()->getDocument(&root);

  DOM::Node* parent_node = FindInRoot(window, root.get());
  ASSERT_TRUE(parent_node);
  DOM::Node* widget_node = parent_node->getChildren(nullptr)->get(0);
  DOM::Node* root_view_node = widget_node->getChildren(nullptr)->get(0);
  Array<DOM::Node>* root_view_children = root_view_node->getChildren(nullptr);
  ASSERT_TRUE(root_view_children);
  size_t root_children_size = root_view_children->length();
  ASSERT_TRUE(root_children_size >= 2);
  DOM::Node* parent_view_node = root_view_children->get(root_children_size - 2);
  DOM::Node* target_view_node = root_view_children->get(root_children_size - 1);
  DOM::Node* child_view_node = parent_view_node->getChildren(nullptr)->get(0);

  Compare(parent_view, parent_view_node);
  Compare(target_view, target_view_node);
  Compare(child_view, child_view_node);
  parent_view->RemoveChildView(child_view);
  target_view->AddChildView(child_view);
  ExpectChildNodeRemoved(parent_view_node->getNodeId(),
                         child_view_node->getNodeId());
  ExpectChildNodeInserted(target_view_node->getNodeId(), 0);
}

TEST_F(UIDevToolsTest, WindowWidgetViewHighlight) {
  std::unique_ptr<views::Widget> widget(
      CreateTestWidget(gfx::Rect(0, 0, 400, 400)));
  aura::Window* parent_window = widget->GetNativeWindow();
  std::unique_ptr<aura::Window> window(CreateChildWindow(parent_window));
  views::View* root_view = widget->GetRootView();

  std::unique_ptr<DOM::Node> root;
  dom_agent()->getDocument(&root);

  DOM::Node* parent_node = FindInRoot(parent_window, root.get());
  ASSERT_TRUE(parent_node);
  Array<DOM::Node>* parent_children = parent_node->getChildren(nullptr);
  ASSERT_TRUE(parent_children);
  DOM::Node* window_node = parent_children->get(1);
  DOM::Node* widget_node = parent_children->get(0);
  DOM::Node* root_view_node = widget_node->getChildren(nullptr)->get(0);

  HighlightNode(window_node->getNodeId());

  ui::Layer* highlighting_layer = GetHighlightingLayer(GetPrimaryRootWindow());
  EXPECT_TRUE(highlighting_layer->visible());
  EXPECT_EQ(kBackgroundColor, highlighting_layer->GetTargetColor());

  UIElement* element =
      dom_agent()->GetElementFromNodeId(window_node->getNodeId());
  ASSERT_EQ(UIElementType::WINDOW, element->type());
  EXPECT_EQ(element->GetNodeWindowAndBounds().first, window.get());
  EXPECT_EQ(element->GetNodeWindowAndBounds().second,
            window->GetBoundsInScreen());

  HideHighlight(0);
  HighlightNode(widget_node->getNodeId());

  highlighting_layer = GetHighlightingLayer(GetPrimaryRootWindow());
  EXPECT_TRUE(highlighting_layer->visible());
  EXPECT_EQ(kBackgroundColor, highlighting_layer->GetTargetColor());

  element = dom_agent()->GetElementFromNodeId(widget_node->getNodeId());
  ASSERT_EQ(UIElementType::WIDGET, element->type());
  EXPECT_EQ(element->GetNodeWindowAndBounds().first, widget->GetNativeWindow());
  EXPECT_EQ(element->GetNodeWindowAndBounds().second,
            widget->GetWindowBoundsInScreen());

  HideHighlight(0);
  HighlightNode(root_view_node->getNodeId());

  highlighting_layer = GetHighlightingLayer(GetPrimaryRootWindow());
  EXPECT_TRUE(highlighting_layer->visible());
  EXPECT_EQ(kBackgroundColor, highlighting_layer->GetTargetColor());

  element = dom_agent()->GetElementFromNodeId(root_view_node->getNodeId());
  ASSERT_EQ(UIElementType::VIEW, element->type());
  EXPECT_EQ(element->GetNodeWindowAndBounds().first,
            root_view->GetWidget()->GetNativeWindow());
  EXPECT_EQ(element->GetNodeWindowAndBounds().second,
            root_view->GetBoundsInScreen());

  HideHighlight(0);

  // Highlight non-existent node
  HighlightNode(10000);
  EXPECT_FALSE(GetHighlightingLayer(GetPrimaryRootWindow())->visible());
}

int GetNodeIdFromWindow(UIElement* ui_element, aura::Window* window) {
  for (auto* child : ui_element->children()) {
    if (child->type() == UIElementType::WINDOW &&
        static_cast<WindowElement*>(child)->window() == window) {
      return child->node_id();
    }
  }
  for (auto* child : ui_element->children()) {
    if (child->type() == UIElementType::WINDOW) {
      int node_id = GetNodeIdFromWindow(child, window);
      if (node_id > 0)
        return node_id;
    }
  }
  return 0;
}

// TODO(thanhph): Make test AshDevToolsTest.MultipleDisplayHighlight work with
// multiple displays. https://crbug.com/726831.

TEST_F(UIDevToolsTest, WindowWidgetViewGetMatchedStylesForNode) {
  std::unique_ptr<views::Widget> widget(
      CreateTestWidget(gfx::Rect(1, 1, 1, 1)));
  aura::Window* parent_window = widget->GetNativeWindow();
  std::unique_ptr<aura::Window> window(CreateChildWindow(parent_window));
  gfx::Rect window_bounds(2, 2, 3, 3);
  gfx::Rect widget_bounds(50, 50, 100, 75);
  gfx::Rect view_bounds(4, 4, 3, 3);
  window->SetBounds(window_bounds);
  widget->SetBounds(widget_bounds);
  widget->GetRootView()->SetBoundsRect(view_bounds);

  std::unique_ptr<DOM::Node> root;
  dom_agent()->getDocument(&root);

  DOM::Node* parent_node = FindInRoot(parent_window, root.get());
  ASSERT_TRUE(parent_node);
  Array<DOM::Node>* parent_children = parent_node->getChildren(nullptr);
  ASSERT_TRUE(parent_children);

  CompareNodeBounds(parent_node, widget_bounds);
  CompareNodeBounds(parent_children->get(1), window_bounds);
  CompareNodeBounds(parent_children->get(0)->getChildren(nullptr)->get(0),
                    view_bounds);
  CompareViewAtrributes(parent_children->get(0)->getChildren(nullptr)->get(0),
                        widget->GetRootView());
}

TEST_F(UIDevToolsTest, WindowWidgetViewStyleSheetChanged) {
  std::unique_ptr<views::Widget> widget(
      CreateTestWidget(gfx::Rect(1, 1, 1, 1)));
  aura::Window* widget_window = widget->GetNativeWindow();
  std::unique_ptr<aura::Window> child(CreateChildWindow(widget_window));

  std::unique_ptr<DOM::Node> root;
  dom_agent()->getDocument(&root);

  gfx::Rect child_bounds(2, 2, 3, 3);
  gfx::Rect widget_bounds(10, 10, 150, 160);
  gfx::Rect view_bounds(4, 4, 3, 3);
  child->SetBounds(child_bounds);
  widget->SetBounds(widget_bounds);
  widget->GetRootView()->SetBoundsRect(view_bounds);

  DOM::Node* widget_node = FindInRoot(widget_window, root.get());
  ASSERT_TRUE(widget_node);
  Array<DOM::Node>* widget_node_children = widget_node->getChildren(nullptr);
  ASSERT_TRUE(widget_node_children);

  EXPECT_EQ(1, GetStyleSheetChangedCount(widget_node->getNodeId()));
  EXPECT_EQ(
      1, GetStyleSheetChangedCount(widget_node_children->get(1)->getNodeId()));
  EXPECT_EQ(2, GetStyleSheetChangedCount(widget_node_children->get(0)
                                             ->getChildren(nullptr)
                                             ->get(0)
                                             ->getNodeId()));
}

TEST_F(UIDevToolsTest, WindowWidgetViewSetStyleText) {
  std::unique_ptr<views::Widget> widget(
      CreateTestWidget(gfx::Rect(0, 0, 400, 400)));
  aura::Window* parent_window = widget->GetNativeWindow();
  std::unique_ptr<aura::Window> window(CreateChildWindow(parent_window));
  views::View* root_view = widget->GetRootView();

  std::unique_ptr<DOM::Node> root;
  dom_agent()->getDocument(&root);

  DOM::Node* parent_node = FindInRoot(parent_window, root.get());
  ASSERT_TRUE(parent_node);
  Array<DOM::Node>* parent_children = parent_node->getChildren(nullptr);
  ASSERT_TRUE(parent_children);

  // Test different combinations on window node
  DOM::Node* window_node = parent_children->get(1);

  SetStyleTexts(window_node,
                "x: 25; y:35; width: 5; height: 20; visibility: 1;", true);
  EXPECT_EQ(gfx::Rect(25, 35, 5, 20), window->bounds());
  EXPECT_TRUE(window->IsVisible());

  SetStyleTexts(window_node, "test_nothing_happens:1;", false);
  EXPECT_EQ(gfx::Rect(25, 35, 5, 20), window->bounds());  // Not changed

  SetStyleTexts(window_node, "\nheight: 10;\nvisibility: 0;\n", true);
  EXPECT_EQ(gfx::Rect(25, 35, 5, 10), window->bounds());
  EXPECT_FALSE(window->IsVisible());

  SetStyleTexts(window_node, "\nx: 10; y: 23; width: 52;\n  ", true);
  EXPECT_EQ(gfx::Rect(10, 23, 52, 10), window->bounds());

  // Test different combinations on widget node
  DOM::Node* widget_node = parent_children->get(0);

  SetStyleTexts(widget_node,
                "x: 25; y:35; width: 53; height: 64; visibility: 0;", true);
  EXPECT_EQ(gfx::Rect(25, 35, 53, 64), widget->GetRestoredBounds());
  EXPECT_FALSE(widget->IsVisible());

  SetStyleTexts(widget_node, "test_nothing_happens:1;", false);
  EXPECT_EQ(gfx::Rect(25, 35, 53, 64),
            widget->GetRestoredBounds());  // Not changed

  SetStyleTexts(widget_node, "\nheight: 123;\nvisibility: 1;\n", true);
  EXPECT_EQ(gfx::Rect(25, 35, 53, 123), widget->GetRestoredBounds());
  EXPECT_TRUE(widget->IsVisible());

  SetStyleTexts(widget_node, "\nx: 10; y: 23; width: 98;\n  ", true);
  EXPECT_EQ(gfx::Rect(10, 23, 98, 123), widget->GetRestoredBounds());

  // Test different combinations on view node
  DOM::Node* root_view_node = widget_node->getChildren(nullptr)->get(0);

  SetStyleTexts(root_view_node,
                "x: 25; y:35; width: 45; height: 20; visibility: 0;", true);
  EXPECT_EQ(gfx::Rect(25, 35, 45, 20), root_view->bounds());
  EXPECT_FALSE(root_view->visible());

  SetStyleTexts(root_view_node, "test_nothing_happens:1;", false);
  EXPECT_EQ(gfx::Rect(25, 35, 45, 20), root_view->bounds());  // Not changed

  SetStyleTexts(root_view_node, "\nheight: 73;\n  ", true);
  EXPECT_EQ(gfx::Rect(25, 35, 45, 73), root_view->bounds());

  SetStyleTexts(root_view_node, "\nx: 10; y: 23; width: 52;\nvisibility: 1;\n",
                true);
  EXPECT_EQ(gfx::Rect(10, 23, 52, 73), root_view->bounds());
  EXPECT_TRUE(root_view->visible());
}

}  // namespace ui_devtools
