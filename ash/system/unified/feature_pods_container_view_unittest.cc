// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/unified/feature_pods_container_view.h"

#include "ash/system/tray/tray_constants.h"
#include "ash/system/unified/feature_pod_button.h"
#include "ash/system/unified/feature_pod_controller_base.h"
#include "ash/test/ash_test_base.h"
#include "ui/views/view_observer.h"

namespace ash {

class FeaturePodsContainerViewTest : public AshTestBase,
                                     public FeaturePodControllerBase,
                                     public views::ViewObserver {
 public:
  FeaturePodsContainerViewTest() = default;
  ~FeaturePodsContainerViewTest() override = default;

  // AshTestBase:
  void SetUp() override {
    AshTestBase::SetUp();
    container_ = std::make_unique<FeaturePodsContainerView>(
        true /* initially_expanded */);
    container_->AddObserver(this);
  }

  // FeaturePodControllerBase:
  FeaturePodButton* CreateButton() override { return nullptr; }
  void OnIconPressed() override {}

  // views::ViewObserver:
  void OnViewPreferredSizeChanged(views::View* observed_view) override {
    container_->SetBoundsRect(gfx::Rect(container_->GetPreferredSize()));
    container_->Layout();
  }

 protected:
  void AddButtons(int count) {
    for (int i = 0; i < count; ++i) {
      buttons_.push_back(new FeaturePodButton(this));
      container()->AddChildView(buttons_.back());
    }
    OnViewPreferredSizeChanged(container());
  }

  FeaturePodsContainerView* container() { return container_.get(); }

  std::vector<FeaturePodButton*> buttons_;

 private:
  std::unique_ptr<FeaturePodsContainerView> container_;

  DISALLOW_COPY_AND_ASSIGN(FeaturePodsContainerViewTest);
};

TEST_F(FeaturePodsContainerViewTest, ExpandedAndCollapsed) {
  const int kNumberOfAddedButtons = kUnifiedFeaturePodItemsInRow * 3;
  EXPECT_LT(kUnifiedFeaturePodMaxItemsInCollapsed, kNumberOfAddedButtons);

  AddButtons(kNumberOfAddedButtons);

  // In expanded state, buttons are laid out in plane.
  EXPECT_LT(buttons_[0]->x(), buttons_[1]->x());
  EXPECT_EQ(buttons_[0]->y(), buttons_[1]->y());
  // If the row exceeds kUnifiedFeaturePodItemsInRow, the next button is placed
  // right under the first button.
  EXPECT_EQ(buttons_[0]->x(), buttons_[kUnifiedFeaturePodItemsInRow]->x());
  EXPECT_LT(buttons_[0]->y(), buttons_[kUnifiedFeaturePodItemsInRow]->y());
  // All buttons are visible.
  for (auto* button : buttons_)
    EXPECT_TRUE(button->visible());

  container()->SetExpandedAmount(0.0);

  // In collapsed state, all buttons are laid out horizontally.
  for (int i = 1; i < kUnifiedFeaturePodMaxItemsInCollapsed; ++i)
    EXPECT_EQ(buttons_[0]->y(), buttons_[i]->y());

  // Buttons exceed kUnifiedFeaturePodMaxItemsInCollapsed are invisible.
  for (int i = 0; i < kNumberOfAddedButtons; ++i) {
    EXPECT_EQ(i < kUnifiedFeaturePodMaxItemsInCollapsed,
              buttons_[i]->visible());
  }
}

TEST_F(FeaturePodsContainerViewTest, HiddenButtonRemainsHidden) {
  AddButtons(kUnifiedFeaturePodMaxItemsInCollapsed);
  // The button is invisible in expanded state.
  buttons_.front()->SetVisible(false);
  container()->SetExpandedAmount(0.0);
  EXPECT_FALSE(buttons_.front()->visible());
  container()->SetExpandedAmount(1.0);
  EXPECT_FALSE(buttons_.front()->visible());
}

TEST_F(FeaturePodsContainerViewTest, BecomeVisibleInCollapsed) {
  AddButtons(kUnifiedFeaturePodMaxItemsInCollapsed);
  // The button is invisible in expanded state.
  buttons_.back()->SetVisible(false);
  container()->SetExpandedAmount(0.0);
  // The button becomes visible in collapsed state.
  buttons_.back()->SetVisible(true);
  // As the container still has remaining space, the button will be visible.
  EXPECT_TRUE(buttons_.back()->visible());
}

TEST_F(FeaturePodsContainerViewTest, StillHiddenInCollapsed) {
  AddButtons(kUnifiedFeaturePodMaxItemsInCollapsed + 1);
  // The button is invisible in expanded state.
  buttons_.back()->SetVisible(false);
  container()->SetExpandedAmount(0.0);
  // The button becomes visible in collapsed state.
  buttons_.back()->SetVisible(true);
  // As the container doesn't have remaining space, the button won't be visible.
  EXPECT_FALSE(buttons_.back()->visible());

  container()->SetExpandedAmount(1.0);
  // The button becomes visible in expanded state.
  EXPECT_TRUE(buttons_.back()->visible());
}

TEST_F(FeaturePodsContainerViewTest, DifferentButtonBecomeVisibleInCollapsed) {
  AddButtons(kUnifiedFeaturePodMaxItemsInCollapsed + 1);
  container()->SetExpandedAmount(0.0);
  // The last button is not visible as it doesn't have enough space.
  EXPECT_FALSE(buttons_.back()->visible());
  // The first button becomes invisible.
  buttons_.front()->SetVisible(false);
  // The last button now has the space for it.
  EXPECT_TRUE(buttons_.back()->visible());
}

}  // namespace ash
