// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/login/ui/login_expanded_public_account_view.h"
#include "ash/login/ui/login_test_base.h"
#include "ash/login/ui/login_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/events/test/event_generator.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/widget/widget.h"

namespace ash {

namespace {

// Total width of the expanded view.
constexpr int kBubbleTotalWidthDp = 600;
// Total height of the expanded view.
constexpr int kBubbleTotalHeightDp = 324;

class LoginExpandedPublicAccountViewTest : public LoginTestBase {
 protected:
  LoginExpandedPublicAccountViewTest() = default;
  ~LoginExpandedPublicAccountViewTest() override = default;

  // LoginTestBase:
  void SetUp() override {
    LoginTestBase::SetUp();

    user_ = CreatePublicAccountUser("user@domain.com");
    view_ = new LoginExpandedPublicAccountView(base::DoNothing());
    view_->UpdateForUser(user_);

    container_ = new views::View();
    container_->SetLayoutManager(
        std::make_unique<views::BoxLayout>(views::BoxLayout::kHorizontal));
    container_->AddChildView(view_);
    SetWidget(CreateWidgetWithContent(container_));
  }

  mojom::LoginUserInfoPtr user_;
  // Owned by test widget view hierarchy.
  views::View* container_ = nullptr;
  // Owned by test widget view hierarchy
  LoginExpandedPublicAccountView* view_ = nullptr;

 private:
  DISALLOW_COPY_AND_ASSIGN(LoginExpandedPublicAccountViewTest);
};

}  // namespace

// Verifies toggle advanced view will update the layout correctly.
TEST_F(LoginExpandedPublicAccountViewTest, ToggleAdvancedView) {
  view_->SizeToPreferredSize();
  EXPECT_EQ(view_->width(), kBubbleTotalWidthDp);
  EXPECT_EQ(view_->height(), kBubbleTotalHeightDp);

  LoginExpandedPublicAccountView::TestApi test_api(view_);
  EXPECT_FALSE(user_->public_account_info->show_advanced_view);
  EXPECT_FALSE(test_api.advanced_view()->visible());

  // Toggle show_advanced_view.
  user_->public_account_info->show_advanced_view = true;
  view_->UpdateForUser(user_);

  // Advanced view is shown and the overall size does not change.
  EXPECT_TRUE(test_api.advanced_view()->visible());
  EXPECT_EQ(view_->width(), kBubbleTotalWidthDp);
  EXPECT_EQ(view_->height(), kBubbleTotalHeightDp);

  // Click on the show advanced button.
  ui::test::EventGenerator& generator = GetEventGenerator();
  generator.MoveMouseTo(
      test_api.advanced_view_button()->GetBoundsInScreen().CenterPoint());
  generator.ClickLeftButton();

  // Advanced view is hidden and the overall size does not change.
  EXPECT_FALSE(test_api.advanced_view()->visible());
  EXPECT_EQ(view_->width(), kBubbleTotalWidthDp);
  EXPECT_EQ(view_->height(), kBubbleTotalHeightDp);
}

}  // namespace ash
