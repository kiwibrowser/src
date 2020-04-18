// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/win/direct_manipulation.h"

#include <windows.h>

#include "base/macros.h"
#include "base/test/scoped_feature_list.h"
#include "base/win/windows_version.h"
#include "content/browser/renderer_host/legacy_render_widget_host_win.h"
#include "content/browser/renderer_host/render_widget_host_view_aura.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/shell/browser/shell.h"
#include "ui/aura/window.h"
#include "ui/aura/window_tree_host.h"
#include "ui/base/ui_base_features.h"
#include "ui/base/win/window_event_target.h"
#include "ui/events/event_rewriter.h"
#include "ui/events/event_source.h"
#include "url/gurl.h"

namespace content {

class DirectManipulationBrowserTest : public ContentBrowserTest,
                                      public testing::WithParamInterface<bool> {
 public:
  DirectManipulationBrowserTest() {
    if (GetParam()) {
      scoped_feature_list_.InitWithFeatures(
          {features::kPrecisionTouchpad,
           features::kPrecisionTouchpadScrollPhase},
          {});
    } else {
      scoped_feature_list_.InitWithFeatures(
          {features::kPrecisionTouchpad},
          {features::kPrecisionTouchpadScrollPhase});
    }
  }

  ~DirectManipulationBrowserTest() override {}

  LegacyRenderWidgetHostHWND* GetLegacyRenderWidgetHostHWND() {
    RenderWidgetHostViewAura* rwhva = static_cast<RenderWidgetHostViewAura*>(
        shell()->web_contents()->GetRenderWidgetHostView());
    return rwhva->legacy_render_widget_host_HWND_;
  }

  HWND GetSubWindowHWND() {
    LegacyRenderWidgetHostHWND* lrwhh = GetLegacyRenderWidgetHostHWND();

    return lrwhh->hwnd();
  }

  ui::WindowEventTarget* GetWindowEventTarget() {
    LegacyRenderWidgetHostHWND* lrwhh = GetLegacyRenderWidgetHostHWND();

    return lrwhh->GetWindowEventTarget(lrwhh->GetParent());
  }

  void SimulatePointerHitTest() {
    LegacyRenderWidgetHostHWND* lrwhh = GetLegacyRenderWidgetHostHWND();

    lrwhh->direct_manipulation_helper_->need_poll_events_ = true;
    lrwhh->CreateAnimationObserver();
  }

  void UpdateParent(HWND hwnd) {
    LegacyRenderWidgetHostHWND* lrwhh = GetLegacyRenderWidgetHostHWND();

    lrwhh->UpdateParent(hwnd);
  }

  bool HasCompositorAnimationObserver(LegacyRenderWidgetHostHWND* lrwhh) {
    return lrwhh->compositor_animation_observer_ != nullptr;
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;

  DISALLOW_COPY_AND_ASSIGN(DirectManipulationBrowserTest);
};

INSTANTIATE_TEST_CASE_P(WithScrollEventPhase,
                        DirectManipulationBrowserTest,
                        testing::Bool());

// Ensure the AnimationObserver destroy when hwnd reparent to other hwnd.
IN_PROC_BROWSER_TEST_P(DirectManipulationBrowserTest, HWNDReparent) {
  if (base::win::GetVersion() < base::win::VERSION_WIN10)
    return;

  NavigateToURL(shell(), GURL(url::kAboutBlankURL));

  LegacyRenderWidgetHostHWND* lrwhh = GetLegacyRenderWidgetHostHWND();
  ASSERT_TRUE(lrwhh);

  // The observer should not create before it needed.
  ASSERT_TRUE(!HasCompositorAnimationObserver(lrwhh));

  // Add AnimationObserver to tab to simulate direct manipulation start.
  SimulatePointerHitTest();
  ASSERT_TRUE(HasCompositorAnimationObserver(lrwhh));

  // Create another browser.
  Shell* shell2 = CreateBrowser();
  NavigateToURL(shell2, GURL(url::kAboutBlankURL));

  // Move to the tab to browser2.
  UpdateParent(
      shell2->window()->GetRootWindow()->GetHost()->GetAcceleratedWidget());

  // The animation observer should be removed.
  EXPECT_FALSE(HasCompositorAnimationObserver(lrwhh));

  shell2->Close();
}

// EventLogger is to obserser the events sent from WindowEventTarget (the root
// window).
class EventLogger : public ui::EventRewriter {
 public:
  EventLogger() {}
  ~EventLogger() override {}

  std::unique_ptr<ui::Event> ReleaseLastEvent() {
    return std::move(last_event_);
  }

 private:
  // ui::EventRewriter
  ui::EventRewriteStatus RewriteEvent(
      const ui::Event& event,
      std::unique_ptr<ui::Event>* new_event) override {
    DCHECK(!last_event_);
    last_event_ = ui::Event::Clone(event);
    return ui::EVENT_REWRITE_CONTINUE;
  }

  // ui::EventRewriter
  ui::EventRewriteStatus NextDispatchEvent(
      const ui::Event& last_event,
      std::unique_ptr<ui::Event>* new_event) override {
    return ui::EVENT_REWRITE_CONTINUE;
  }

  std::unique_ptr<ui::Event> last_event_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(EventLogger);
};

// Check DirectManipulation events convert to ui::event correctly.
IN_PROC_BROWSER_TEST_P(DirectManipulationBrowserTest, EventConvert) {
  if (base::win::GetVersion() < base::win::VERSION_WIN10)
    return;

  NavigateToURL(shell(), GURL(url::kAboutBlankURL));

  LegacyRenderWidgetHostHWND* lrwhh = GetLegacyRenderWidgetHostHWND();
  ASSERT_TRUE(lrwhh);

  HWND hwnd =
      shell()->window()->GetRootWindow()->GetHost()->GetAcceleratedWidget();

  ui::EventSource* dwthw = static_cast<ui::EventSource*>(
      aura::WindowTreeHost::GetForAcceleratedWidget(hwnd));
  EventLogger event_logger;
  dwthw->AddEventRewriter(&event_logger);

  ui::WindowEventTarget* target = GetWindowEventTarget();

  {
    target->ApplyPanGestureScroll(1, 2);
    std::unique_ptr<ui::Event> event = event_logger.ReleaseLastEvent();
    ASSERT_TRUE(event);

    if (GetParam()) {
      EXPECT_EQ(ui::ET_SCROLL, event->type());
      ui::ScrollEvent* scroll_event = event->AsScrollEvent();
      EXPECT_EQ(1, scroll_event->x_offset());
      EXPECT_EQ(2, scroll_event->y_offset());
      EXPECT_EQ(ui::EventMomentumPhase::NONE, scroll_event->momentum_phase());
      EXPECT_EQ(ui::ScrollEventPhase::kUpdate,
                scroll_event->scroll_event_phase());
    } else {
      EXPECT_EQ(ui::ET_MOUSEWHEEL, event->type());
      ui::MouseWheelEvent* wheel_event = event->AsMouseWheelEvent();
      EXPECT_EQ(1, wheel_event->x_offset());
      EXPECT_EQ(2, wheel_event->y_offset());
      EXPECT_TRUE(wheel_event->flags() & ui::EF_PRECISION_SCROLLING_DELTA);
    }
  }

  {
    target->ApplyPanGestureFling(1, 2);
    std::unique_ptr<ui::Event> event = event_logger.ReleaseLastEvent();
    ASSERT_TRUE(event);

    if (GetParam()) {
      EXPECT_EQ(ui::ET_SCROLL, event->type());
      ui::ScrollEvent* scroll_event = event->AsScrollEvent();
      EXPECT_EQ(1, scroll_event->x_offset());
      EXPECT_EQ(2, scroll_event->y_offset());
      EXPECT_EQ(ui::EventMomentumPhase::INERTIAL_UPDATE,
                scroll_event->momentum_phase());
      EXPECT_EQ(ui::ScrollEventPhase::kNone,
                scroll_event->scroll_event_phase());
    } else {
      EXPECT_EQ(ui::ET_MOUSEWHEEL, event->type());
      ui::MouseWheelEvent* wheel_event = event->AsMouseWheelEvent();
      EXPECT_EQ(1, wheel_event->x_offset());
      EXPECT_EQ(2, wheel_event->y_offset());
      EXPECT_TRUE(wheel_event->flags() & ui::EF_PRECISION_SCROLLING_DELTA);
    }
  }

  {
    target->ApplyPanGestureScrollBegin(1, 2);
    std::unique_ptr<ui::Event> event = event_logger.ReleaseLastEvent();

    if (GetParam()) {
      ASSERT_TRUE(event);
      EXPECT_EQ(ui::ET_SCROLL, event->type());
      ui::ScrollEvent* scroll_event = event->AsScrollEvent();
      EXPECT_EQ(1, scroll_event->x_offset());
      EXPECT_EQ(2, scroll_event->y_offset());
      EXPECT_EQ(ui::EventMomentumPhase::NONE, scroll_event->momentum_phase());
      EXPECT_EQ(ui::ScrollEventPhase::kBegan,
                scroll_event->scroll_event_phase());
    } else {
      EXPECT_EQ(ui::ET_MOUSEWHEEL, event->type());
      ui::MouseWheelEvent* wheel_event = event->AsMouseWheelEvent();
      EXPECT_EQ(1, wheel_event->x_offset());
      EXPECT_EQ(2, wheel_event->y_offset());
      EXPECT_TRUE(wheel_event->flags() & ui::EF_PRECISION_SCROLLING_DELTA);
    }
  }

  {
    target->ApplyPanGestureScrollEnd();
    std::unique_ptr<ui::Event> event = event_logger.ReleaseLastEvent();

    if (GetParam()) {
      ASSERT_TRUE(event);
      EXPECT_EQ(ui::ET_SCROLL, event->type());
      ui::ScrollEvent* scroll_event = event->AsScrollEvent();
      EXPECT_EQ(0, scroll_event->x_offset());
      EXPECT_EQ(0, scroll_event->y_offset());
      EXPECT_EQ(ui::EventMomentumPhase::NONE, scroll_event->momentum_phase());
      EXPECT_EQ(ui::ScrollEventPhase::kEnd, scroll_event->scroll_event_phase());
    } else {
      ASSERT_FALSE(event);
    }
  }

  {
    target->ApplyPanGestureFlingBegin();
    std::unique_ptr<ui::Event> event = event_logger.ReleaseLastEvent();

    if (GetParam()) {
      ASSERT_TRUE(event);
      EXPECT_EQ(ui::ET_SCROLL, event->type());
      ui::ScrollEvent* scroll_event = event->AsScrollEvent();
      EXPECT_EQ(0, scroll_event->x_offset());
      EXPECT_EQ(0, scroll_event->y_offset());
      EXPECT_EQ(ui::EventMomentumPhase::BEGAN, scroll_event->momentum_phase());
      EXPECT_EQ(ui::ScrollEventPhase::kNone,
                scroll_event->scroll_event_phase());
    } else {
      ASSERT_FALSE(event);
    }
  }

  {
    target->ApplyPanGestureFlingEnd();
    std::unique_ptr<ui::Event> event = event_logger.ReleaseLastEvent();

    if (GetParam()) {
      ASSERT_TRUE(event);
      EXPECT_EQ(ui::ET_SCROLL, event->type());
      ui::ScrollEvent* scroll_event = event->AsScrollEvent();
      EXPECT_EQ(0, scroll_event->x_offset());
      EXPECT_EQ(0, scroll_event->y_offset());
      EXPECT_EQ(ui::EventMomentumPhase::END, scroll_event->momentum_phase());
      EXPECT_EQ(ui::ScrollEventPhase::kNone,
                scroll_event->scroll_event_phase());
    } else {
      ASSERT_FALSE(event);
    }
  }

  {
    target->ApplyPinchZoomBegin();
    std::unique_ptr<ui::Event> event = event_logger.ReleaseLastEvent();
    ASSERT_TRUE(event);
    EXPECT_EQ(ui::ET_GESTURE_PINCH_BEGIN, event->type());
    ui::GestureEvent* gesture_event = event->AsGestureEvent();
    EXPECT_EQ(ui::GestureDeviceType::DEVICE_TOUCHPAD,
              gesture_event->details().device_type());
  }

  {
    target->ApplyPinchZoomScale(1.1f);
    std::unique_ptr<ui::Event> event = event_logger.ReleaseLastEvent();
    ASSERT_TRUE(event);
    EXPECT_EQ(ui::ET_GESTURE_PINCH_UPDATE, event->type());
    ui::GestureEvent* gesture_event = event->AsGestureEvent();
    EXPECT_EQ(ui::GestureDeviceType::DEVICE_TOUCHPAD,
              gesture_event->details().device_type());
    EXPECT_EQ(1.1f, gesture_event->details().scale());
  }

  {
    target->ApplyPinchZoomEnd();
    std::unique_ptr<ui::Event> event = event_logger.ReleaseLastEvent();
    ASSERT_TRUE(event);
    EXPECT_EQ(ui::ET_GESTURE_PINCH_END, event->type());
    ui::GestureEvent* gesture_event = event->AsGestureEvent();
    EXPECT_EQ(ui::GestureDeviceType::DEVICE_TOUCHPAD,
              gesture_event->details().device_type());
  }

  dwthw->RemoveEventRewriter(&event_logger);
}

}  // namespace content
