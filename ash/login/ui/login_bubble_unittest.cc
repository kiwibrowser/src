// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <utility>

#include "ash/login/ui/login_bubble.h"
#include "ash/login/ui/login_button.h"
#include "ash/login/ui/login_menu_view.h"
#include "ash/login/ui/login_test_base.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/bind_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/events/test/event_generator.h"
#include "ui/views/animation/test/ink_drop_host_view_test_api.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/widget/widget.h"

namespace ash {

namespace {

// Total width of the bubble view.
constexpr int kBubbleTotalWidthDp = 178;

// Horizontal margin of the bubble view.
constexpr int kBubbleHorizontalMarginDp = 14;

// Top margin of the bubble view.
constexpr int kBubbleTopMarginDp = 13;

// Bottom margin of the bubble view.
constexpr int kBubbleBottomMarginDp = 18;

// Non zero size for the bubble anchor view.
constexpr int kBubbleAnchorViewSizeDp = 100;

std::vector<LoginMenuView::Item> PopulateMenuItems() {
  std::vector<LoginMenuView::Item> items;
  // Add one regular item.
  LoginMenuView::Item item1;
  item1.title = "Regular Item 1";
  item1.is_group = false;
  item1.selected = true;
  items.push_back(item1);

  // Add one group item.
  LoginMenuView::Item item2;
  item2.title = "Group Item 2";
  item2.is_group = true;
  items.push_back(item2);

  // Add another regular item.
  LoginMenuView::Item item3;
  item3.title = "Regular Item 2";
  item3.is_group = false;
  items.push_back(item3);
  return items;
}

class LoginBubbleTest : public LoginTestBase {
 protected:
  LoginBubbleTest() = default;
  ~LoginBubbleTest() override = default;

  // LoginTestBase:
  void SetUp() override {
    LoginTestBase::SetUp();

    container_ = new views::View();
    container_->SetLayoutManager(
        std::make_unique<views::BoxLayout>(views::BoxLayout::kVertical));

    bubble_opener_ = new LoginButton(nullptr /*listener*/);
    other_view_ = new views::View();
    bubble_opener_->SetFocusBehavior(views::View::FocusBehavior::ALWAYS);
    other_view_->SetFocusBehavior(views::View::FocusBehavior::ALWAYS);
    other_view_->SetPreferredSize(
        gfx::Size(kBubbleAnchorViewSizeDp, kBubbleAnchorViewSizeDp));
    bubble_opener_->SetPreferredSize(
        gfx::Size(kBubbleAnchorViewSizeDp, kBubbleAnchorViewSizeDp));

    container_->AddChildView(bubble_opener_);
    container_->AddChildView(other_view_);
    SetWidget(CreateWidgetWithContent(container_));

    bubble_ = std::make_unique<LoginBubble>();
  }

  void TearDown() override {
    bubble_.reset();
    LoginTestBase::TearDown();
  }

  void ShowUserMenu(base::OnceClosure on_remove_show_warning,
                    base::OnceClosure on_remove) {
    bool show_remove_user = !on_remove.is_null();
    bubble_->ShowUserMenu(
        base::string16() /*username*/, base::string16() /*email*/,
        user_manager::UserType::USER_TYPE_REGULAR, false /*is_owner*/,
        container_, bubble_opener_, show_remove_user,
        std::move(on_remove_show_warning), std::move(on_remove));
  }

  void ShowSelectionMenu(const LoginMenuView::OnSelect& on_select) {
    LoginMenuView* view =
        new LoginMenuView(PopulateMenuItems(), container_, on_select);
    bubble_->ShowSelectionMenu(view, bubble_opener_);
  }

  // Owned by test widget view hierarchy.
  views::View* container_ = nullptr;
  // Owned by test widget view hierarchy.
  LoginButton* bubble_opener_ = nullptr;
  // Owned by test widget view hierarchy.
  views::View* other_view_ = nullptr;

  std::unique_ptr<LoginBubble> bubble_;

 private:
  DISALLOW_COPY_AND_ASSIGN(LoginBubbleTest);
};

}  // namespace

// Verifies the base bubble settings.
TEST_F(LoginBubbleTest, BaseBubbleSettings) {
  bubble_->ShowTooltip(base::string16(), bubble_opener_);
  EXPECT_TRUE(bubble_->IsVisible());

  LoginBaseBubbleView* bubble_view = bubble_->bubble_view();
  EXPECT_EQ(bubble_view->GetDialogButtons(), ui::DIALOG_BUTTON_NONE);
  EXPECT_EQ(bubble_view->width(), kBubbleTotalWidthDp);
  EXPECT_EQ(bubble_view->color(), SK_ColorBLACK);
  EXPECT_EQ(bubble_view->margins(),
            gfx::Insets(kBubbleTopMarginDp, kBubbleHorizontalMarginDp,
                        kBubbleBottomMarginDp, kBubbleHorizontalMarginDp));
  bubble_->Close();
}

// Verifies the bubble handles key event correctly.
TEST_F(LoginBubbleTest, BubbleKeyEventHandling) {
  EXPECT_FALSE(bubble_->IsVisible());

  // Verifies that key event won't open the bubble.
  ui::test::EventGenerator& generator = GetEventGenerator();
  other_view_->RequestFocus();
  generator.PressKey(ui::KeyboardCode::VKEY_A, ui::EF_NONE);
  EXPECT_FALSE(bubble_->IsVisible());

  // Verifies that key event on the bubble opener view won't close the bubble.
  ShowUserMenu(base::OnceClosure(), base::OnceClosure());
  EXPECT_TRUE(bubble_->IsVisible());
  bubble_opener_->RequestFocus();
  generator.PressKey(ui::KeyboardCode::VKEY_A, ui::EF_NONE);
  EXPECT_TRUE(bubble_->IsVisible());

  // Verifies that key event on the other view will close the bubble.
  other_view_->RequestFocus();
  generator.PressKey(ui::KeyboardCode::VKEY_A, ui::EF_NONE);
  EXPECT_FALSE(bubble_->IsVisible());
}

// Verifies the bubble handles mouse event correctly.
TEST_F(LoginBubbleTest, BubbleMouseEventHandling) {
  EXPECT_FALSE(bubble_->IsVisible());

  // Verifies that mouse event won't open the bubble.
  ui::test::EventGenerator& generator = GetEventGenerator();
  generator.MoveMouseTo(other_view_->GetBoundsInScreen().CenterPoint());
  generator.ClickLeftButton();
  EXPECT_FALSE(bubble_->IsVisible());

  // Verifies that mouse event on the bubble opener view won't close the bubble.
  ShowUserMenu(base::OnceClosure(), base::OnceClosure());
  EXPECT_TRUE(bubble_->IsVisible());
  generator.MoveMouseTo(bubble_opener_->GetBoundsInScreen().CenterPoint());
  generator.ClickLeftButton();
  EXPECT_TRUE(bubble_->IsVisible());

  // Verifies that mouse event on the bubble itself won't close the bubble.
  generator.MoveMouseTo(
      bubble_->bubble_view()->GetBoundsInScreen().CenterPoint());
  generator.ClickLeftButton();
  EXPECT_TRUE(bubble_->IsVisible());

  // Verifies that mouse event on the other view will close the bubble.
  generator.MoveMouseTo(other_view_->GetBoundsInScreen().CenterPoint());
  generator.ClickLeftButton();
  EXPECT_FALSE(bubble_->IsVisible());
}

// Verifies the bubble handles gesture event correctly.
TEST_F(LoginBubbleTest, BubbleGestureEventHandling) {
  EXPECT_FALSE(bubble_->IsVisible());

  // Verifies that gesture event won't open the bubble.
  ui::test::EventGenerator& generator = GetEventGenerator();
  generator.GestureTapAt(other_view_->GetBoundsInScreen().CenterPoint());
  EXPECT_FALSE(bubble_->IsVisible());

  // Verifies that gesture event on the bubble opener view won't close the
  // bubble.
  ShowUserMenu(base::OnceClosure(), base::OnceClosure());
  EXPECT_TRUE(bubble_->IsVisible());
  generator.GestureTapAt(bubble_opener_->GetBoundsInScreen().CenterPoint());
  EXPECT_TRUE(bubble_->IsVisible());

  // Verifies that gesture event on the bubble itself won't close the bubble.
  generator.GestureTapAt(
      bubble_->bubble_view()->GetBoundsInScreen().CenterPoint());
  EXPECT_TRUE(bubble_->IsVisible());

  // Verifies that gesture event on the other view will close the bubble.
  generator.GestureTapAt(other_view_->GetBoundsInScreen().CenterPoint());
  EXPECT_FALSE(bubble_->IsVisible());
}

// Verifies the ripple effects for the login button.
TEST_F(LoginBubbleTest, LoginButtonRipple) {
  views::test::InkDropHostViewTestApi ink_drop_api(bubble_opener_);
  EXPECT_EQ(ink_drop_api.ink_drop_mode(),
            views::InkDropHostView::InkDropMode::ON);

  // Show the bubble to activate the ripple effect.
  ShowUserMenu(base::OnceClosure(), base::OnceClosure());
  EXPECT_TRUE(bubble_->IsVisible());
  EXPECT_TRUE(ink_drop_api.HasInkDrop());
  EXPECT_EQ(ink_drop_api.GetInkDrop()->GetTargetInkDropState(),
            views::InkDropState::ACTIVATED);
  EXPECT_TRUE(ink_drop_api.GetInkDrop()->IsHighlightFadingInOrVisible());

  // Close the bubble should hide the ripple effect.
  ui::test::EventGenerator& generator = GetEventGenerator();
  generator.MoveMouseTo(other_view_->GetBoundsInScreen().CenterPoint());
  generator.ClickLeftButton();
  EXPECT_FALSE(bubble_->IsVisible());

  // InkDropState::DEACTIVATED state will automatically transition to the
  // InkDropState::HIDDEN state.
  EXPECT_EQ(ink_drop_api.GetInkDrop()->GetTargetInkDropState(),
            views::InkDropState::HIDDEN);
  EXPECT_FALSE(ink_drop_api.GetInkDrop()->IsHighlightFadingInOrVisible());
}

// Verifies that clicking remove user requires two clicks before firing the
// callback.
TEST_F(LoginBubbleTest, RemoveUserRequiresTwoActivations) {
  // Show the user menu.
  bool remove_warning_called = false;
  bool remove_called = false;
  ShowUserMenu(
      base::BindOnce(
          [](bool* remove_warning_called) { *remove_warning_called = true; },
          &remove_warning_called),
      base::BindOnce([](bool* remove_called) { *remove_called = true; },
                     &remove_called));
  EXPECT_TRUE(bubble_->IsVisible());

  // Focus the remove user button.
  views::View* remove_user_button = bubble_->bubble_view()->GetViewByID(
      LoginBubble::kUserMenuRemoveUserButtonIdForTest);
  remove_user_button->RequestFocus();
  EXPECT_TRUE(remove_user_button->HasFocus());

  auto click = [&]() {
    EXPECT_TRUE(remove_user_button->HasFocus());
    GetEventGenerator().PressKey(ui::KeyboardCode::VKEY_RETURN, 0);
  };

  // First click calls remove warning.
  EXPECT_NO_FATAL_FAILURE(click());
  EXPECT_TRUE(remove_warning_called);
  EXPECT_FALSE(remove_called);
  remove_warning_called = false;

  // Second click calls remove.
  EXPECT_NO_FATAL_FAILURE(click());
  EXPECT_FALSE(remove_warning_called);
  EXPECT_TRUE(remove_called);
}

TEST_F(LoginBubbleTest, ErrorBubbleKeyEventHandling) {
  ui::test::EventGenerator& generator = GetEventGenerator();

  EXPECT_FALSE(bubble_->IsVisible());
  views::Label* error_text = new views::Label(base::ASCIIToUTF16("Error text"));
  bubble_->ShowErrorBubble(error_text, container_, LoginBubble::kFlagsNone);
  EXPECT_TRUE(bubble_->IsVisible());

  // Verifies that key event on a view other than error closes the error bubble.
  other_view_->RequestFocus();
  generator.PressKey(ui::KeyboardCode::VKEY_A, ui::EF_NONE);
  EXPECT_FALSE(bubble_->IsVisible());
}

TEST_F(LoginBubbleTest, ErrorBubbleMouseEventHandling) {
  ui::test::EventGenerator& generator = GetEventGenerator();

  EXPECT_FALSE(bubble_->IsVisible());
  views::Label* error_text = new views::Label(base::ASCIIToUTF16("Error text"));
  bubble_->ShowErrorBubble(error_text, container_, LoginBubble::kFlagsNone);
  EXPECT_TRUE(bubble_->IsVisible());

  // Verifies that mouse event on the bubble itself won't close the bubble.
  generator.MoveMouseTo(
      bubble_->bubble_view()->GetBoundsInScreen().CenterPoint());
  generator.ClickLeftButton();
  EXPECT_TRUE(bubble_->IsVisible());

  // Verifies that mouse event on the other view will close the bubble.
  generator.MoveMouseTo(other_view_->GetBoundsInScreen().CenterPoint());
  generator.ClickLeftButton();
  EXPECT_FALSE(bubble_->IsVisible());
}

TEST_F(LoginBubbleTest, ErrorBubbleGestureEventHandling) {
  ui::test::EventGenerator& generator = GetEventGenerator();

  EXPECT_FALSE(bubble_->IsVisible());
  views::Label* error_text = new views::Label(base::ASCIIToUTF16("Error text"));
  bubble_->ShowErrorBubble(error_text, container_, LoginBubble::kFlagsNone);
  EXPECT_TRUE(bubble_->IsVisible());

  // Verifies that gesture event on the bubble itself won't close the bubble.
  generator.GestureTapAt(
      bubble_->bubble_view()->GetBoundsInScreen().CenterPoint());
  EXPECT_TRUE(bubble_->IsVisible());

  // Verifies that gesture event on the other view will close the bubble.
  generator.GestureTapAt(other_view_->GetBoundsInScreen().CenterPoint());
  EXPECT_FALSE(bubble_->IsVisible());
}

TEST_F(LoginBubbleTest, PersistentErrorBubbleEventHandling) {
  ui::test::EventGenerator& generator = GetEventGenerator();

  EXPECT_FALSE(bubble_->IsVisible());
  views::Label* error_text = new views::Label(base::ASCIIToUTF16("Error text"));
  bubble_->ShowErrorBubble(error_text, container_,
                           LoginBubble::kFlagPersistent);
  EXPECT_TRUE(bubble_->IsVisible());

  // Verifies that mouse event on the bubble itself won't close the bubble.
  generator.MoveMouseTo(
      bubble_->bubble_view()->GetBoundsInScreen().CenterPoint());
  generator.ClickLeftButton();
  EXPECT_TRUE(bubble_->IsVisible());

  // Verifies that mouse event on the other view won't close the bubble.
  generator.MoveMouseTo(other_view_->GetBoundsInScreen().CenterPoint());
  generator.ClickLeftButton();
  EXPECT_TRUE(bubble_->IsVisible());

  // Verifies that gesture event on the bubble itself won't close the bubble.
  generator.GestureTapAt(
      bubble_->bubble_view()->GetBoundsInScreen().CenterPoint());
  EXPECT_TRUE(bubble_->IsVisible());

  // Verifies that gesture event on the other view won't close the bubble.
  generator.GestureTapAt(other_view_->GetBoundsInScreen().CenterPoint());
  EXPECT_TRUE(bubble_->IsVisible());

  // Verifies that key event on the other view won't close the bubble.
  other_view_->RequestFocus();
  generator.PressKey(ui::KeyboardCode::VKEY_A, ui::EF_NONE);
  EXPECT_TRUE(bubble_->IsVisible());

  // LoginBubble::Close should close the persistent error bubble.
  bubble_->Close();
  EXPECT_FALSE(bubble_->IsVisible());
}

TEST_F(LoginBubbleTest, TestShowSelectionMenu) {
  ui::test::EventGenerator& generator = GetEventGenerator();

  EXPECT_FALSE(bubble_->IsVisible());
  LoginMenuView::Item selected_item;
  bool selected = false;
  ShowSelectionMenu(base::BindLambdaForTesting([&](LoginMenuView::Item item) {
    selected_item = item;
    selected = true;
  }));
  EXPECT_TRUE(bubble_->IsVisible());

  // Verifies that regular item 1 is selectable.
  LoginMenuView* menu_view =
      static_cast<LoginMenuView*>(bubble_->bubble_view());
  LoginMenuView::TestApi test_api1(menu_view);
  EXPECT_TRUE(test_api1.contents()->child_at(0)->HasFocus());
  generator.PressKey(ui::KeyboardCode::VKEY_RETURN, 0 /*flag*/);
  EXPECT_FALSE(bubble_->IsVisible());
  EXPECT_EQ(selected_item.title, "Regular Item 1");
  EXPECT_TRUE(selected);

  // Verfies that group item 2 is not selectable.
  selected = false;
  ShowSelectionMenu(base::BindLambdaForTesting([&](LoginMenuView::Item item) {
    selected_item = item;
    selected = true;
  }));
  EXPECT_TRUE(bubble_->IsVisible());
  menu_view = static_cast<LoginMenuView*>(bubble_->bubble_view());
  LoginMenuView::TestApi test_api2(menu_view);
  test_api2.contents()->child_at(1)->RequestFocus();
  generator.PressKey(ui::KeyboardCode::VKEY_RETURN, 0 /*flag*/);
  EXPECT_TRUE(bubble_->IsVisible());
  EXPECT_FALSE(selected);

  // Verifies up/down arrow key can navigate menu entries.
  generator.PressKey(ui::KeyboardCode::VKEY_UP, 0 /*flag*/);
  EXPECT_TRUE(test_api2.contents()->child_at(0)->HasFocus());
  generator.PressKey(ui::KeyboardCode::VKEY_UP, 0 /*flag*/);
  EXPECT_TRUE(test_api2.contents()->child_at(0)->HasFocus());

  generator.PressKey(ui::KeyboardCode::VKEY_DOWN, 0 /*flag*/);
  // Group item is skipped in up/down key navigation.
  EXPECT_TRUE(test_api2.contents()->child_at(2)->HasFocus());
  generator.PressKey(ui::KeyboardCode::VKEY_DOWN, 0 /*flag*/);
  EXPECT_TRUE(test_api2.contents()->child_at(2)->HasFocus());
  EXPECT_TRUE(bubble_->IsVisible());

  bubble_->Close();
  EXPECT_FALSE(bubble_->IsVisible());
}

}  // namespace ash
