// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/tray/tray_detailed_view.h"

#include "ash/ash_view_ids.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/system/tray/system_tray.h"
#include "ash/system/tray/system_tray_bubble.h"
#include "ash/system/tray/system_tray_item.h"
#include "ash/system/tray/system_tray_item_detailed_view_delegate.h"
#include "ash/system/tray/tray_constants.h"
#include "ash/system/tray/view_click_listener.h"
#include "ash/test/ash_test_base.h"
#include "base/memory/ptr_util.h"
#include "base/run_loop.h"
#include "base/test/scoped_mock_time_message_loop_task_runner.h"
#include "base/test/test_mock_time_task_runner.h"
#include "ui/events/test/event_generator.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace ash {

namespace {

class TestDetailsView : public TrayDetailedView {
 public:
  explicit TestDetailsView(DetailedViewDelegate* delegate)
      : TrayDetailedView(delegate) {
    // Uses bluetooth label for testing purpose. It can be changed to any
    // string_id.
    CreateTitleRow(IDS_ASH_STATUS_TRAY_BLUETOOTH);
  }

  ~TestDetailsView() override = default;

  void CreateScrollerViews() { CreateScrollableList(); }

  views::View* scroll_content() const {
    return TrayDetailedView::scroll_content();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(TestDetailsView);
};

// Trivial item implementation that tracks its views for testing.
class TestItem : public SystemTrayItem {
 public:
  TestItem()
      : SystemTrayItem(AshTestBase::GetPrimarySystemTray(), UMA_TEST),
        tray_view_(nullptr),
        default_view_(nullptr),
        detailed_view_(nullptr),
        detailed_view_delegate_(
            std::make_unique<SystemTrayItemDetailedViewDelegate>(this)) {}

  // Overridden from SystemTrayItem:
  views::View* CreateTrayView(LoginStatus status) override {
    tray_view_ = new views::View;
    return tray_view_;
  }
  views::View* CreateDefaultView(LoginStatus status) override {
    default_view_ = new views::View;
    default_view_->SetFocusBehavior(views::View::FocusBehavior::ALWAYS);
    return default_view_;
  }
  views::View* CreateDetailedView(LoginStatus status) override {
    detailed_view_ = new TestDetailsView(detailed_view_delegate_.get());
    return detailed_view_;
  }
  void OnTrayViewDestroyed() override { tray_view_ = NULL; }
  void OnDefaultViewDestroyed() override { default_view_ = NULL; }
  void OnDetailedViewDestroyed() override { detailed_view_ = NULL; }

  views::View* tray_view() const { return tray_view_; }
  views::View* default_view() const { return default_view_; }
  TestDetailsView* detailed_view() const { return detailed_view_; }

 private:
  views::View* tray_view_;
  views::View* default_view_;
  TestDetailsView* detailed_view_;
  const std::unique_ptr<DetailedViewDelegate> detailed_view_delegate_;

  DISALLOW_COPY_AND_ASSIGN(TestItem);
};

}  // namespace

class TrayDetailedViewTest : public AshTestBase {
 public:
  TrayDetailedViewTest() = default;
  ~TrayDetailedViewTest() override = default;

  void TransitionFromDetailedToDefaultView(TestDetailsView* detailed) {
    detailed->TransitionToMainView();
    (*scoped_task_runner_)
        ->FastForwardBy(base::TimeDelta::FromMilliseconds(
            kTrayDetailedViewTransitionDelayMs));
  }

  void FocusBackButton(TestDetailsView* detailed) {
    detailed->back_button_->RequestFocus();
  }

  void SetUp() override {
    AshTestBase::SetUp();
    scoped_task_runner_ =
        std::make_unique<base::ScopedMockTimeMessageLoopTaskRunner>();
  }

  void TearDown() override {
    scoped_task_runner_.reset();
    AshTestBase::TearDown();
  }

 private:
  // Used to control the |transition_delay_timer_|.
  std::unique_ptr<base::ScopedMockTimeMessageLoopTaskRunner>
      scoped_task_runner_;

  DISALLOW_COPY_AND_ASSIGN(TrayDetailedViewTest);
};

TEST_F(TrayDetailedViewTest, TransitionToDefaultViewTest) {
  SystemTray* tray = GetPrimarySystemTray();
  ASSERT_TRUE(tray->GetWidget());

  TestItem* test_item_1 = new TestItem;
  TestItem* test_item_2 = new TestItem;
  tray->AddTrayItem(base::WrapUnique(test_item_1));
  tray->AddTrayItem(base::WrapUnique(test_item_2));

  // Ensure the tray views are created.
  ASSERT_TRUE(test_item_1->tray_view() != NULL);
  ASSERT_TRUE(test_item_2->tray_view() != NULL);

  // Show the default view.
  tray->ShowDefaultView(BUBBLE_CREATE_NEW, false /* show_by_click */);
  RunAllPendingInMessageLoop();

  // Show the detailed view of item 2.
  tray->ShowDetailedView(test_item_2, 0, BUBBLE_USE_EXISTING);
  EXPECT_TRUE(test_item_2->detailed_view());
  RunAllPendingInMessageLoop();
  EXPECT_FALSE(test_item_2->default_view());

  // Transition back to default view, the default view of item 2 should have
  // focus.
  tray->GetSystemBubble()->bubble_view()->set_can_activate(true);
  FocusBackButton(test_item_2->detailed_view());
  TransitionFromDetailedToDefaultView(test_item_2->detailed_view());
  RunAllPendingInMessageLoop();

  EXPECT_TRUE(test_item_2->default_view());
  EXPECT_FALSE(test_item_2->detailed_view());
  EXPECT_TRUE(test_item_2->default_view()->HasFocus());

  // Show the detailed view of item 2 again.
  tray->ShowDetailedView(test_item_2, 0, BUBBLE_USE_EXISTING);
  EXPECT_TRUE(test_item_2->detailed_view());
  RunAllPendingInMessageLoop();
  EXPECT_FALSE(test_item_2->default_view());

  // Transition back to default view, the default view of item 2 should NOT have
  // focus.
  TransitionFromDetailedToDefaultView(test_item_2->detailed_view());
  RunAllPendingInMessageLoop();

  EXPECT_TRUE(test_item_2->default_view());
  EXPECT_FALSE(test_item_2->detailed_view());
  EXPECT_FALSE(test_item_2->default_view()->HasFocus());
}

TEST_F(TrayDetailedViewTest, ScrollContentsTest) {
  SystemTray* tray = GetPrimarySystemTray();
  TestItem* test_item = new TestItem;
  tray->AddTrayItem(base::WrapUnique(test_item));
  tray->ShowDefaultView(BUBBLE_CREATE_NEW, false /* show_by_click */);
  RunAllPendingInMessageLoop();
  tray->ShowDetailedView(test_item, 0, BUBBLE_USE_EXISTING);
  RunAllPendingInMessageLoop();
  test_item->detailed_view()->CreateScrollerViews();

  test_item->detailed_view()->scroll_content()->SetPaintToLayer();
  views::View* view1 = new views::View();
  test_item->detailed_view()->scroll_content()->AddChildView(view1);
  views::View* view2 = new views::View();
  view2->SetPaintToLayer();
  test_item->detailed_view()->scroll_content()->AddChildView(view2);
  views::View* view3 = new views::View();
  view3->SetPaintToLayer();
  test_item->detailed_view()->scroll_content()->AddChildView(view3);

  // Child layers should have same order as the child views.
  const std::vector<ui::Layer*>& layers =
      test_item->detailed_view()->scroll_content()->layer()->children();
  EXPECT_EQ(2u, layers.size());
  EXPECT_EQ(view2->layer(), layers[0]);
  EXPECT_EQ(view3->layer(), layers[1]);

  // Mark |view2| as sticky and add one more child (which will reorder layers).
  view2->set_id(VIEW_ID_STICKY_HEADER);
  views::View* view4 = new views::View();
  view4->SetPaintToLayer();
  test_item->detailed_view()->scroll_content()->AddChildView(view4);

  // Sticky header layer should be above the last child's layer.
  EXPECT_EQ(3u, layers.size());
  EXPECT_EQ(view3->layer(), layers[0]);
  EXPECT_EQ(view4->layer(), layers[1]);
  EXPECT_EQ(view2->layer(), layers[2]);
}

}  // namespace ash
