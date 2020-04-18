// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/shelf/shelf_view_test_api.h"

#include "ash/public/cpp/shelf_model.h"
#include "ash/shelf/overflow_button.h"
#include "ash/shelf/shelf_button.h"
#include "ash/shelf/shelf_constants.h"
#include "ash/shelf/shelf_view.h"
#include "base/run_loop.h"
#include "ui/views/animation/bounds_animator.h"
#include "ui/views/controls/menu/menu_runner.h"
#include "ui/views/view_model.h"

namespace {

// A class used to wait for animations.
class TestAPIAnimationObserver : public views::BoundsAnimatorObserver {
 public:
  TestAPIAnimationObserver() = default;
  ~TestAPIAnimationObserver() override = default;

  // views::BoundsAnimatorObserver overrides:
  void OnBoundsAnimatorProgressed(views::BoundsAnimator* animator) override {}
  void OnBoundsAnimatorDone(views::BoundsAnimator* animator) override {
    base::RunLoop::QuitCurrentWhenIdleDeprecated();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(TestAPIAnimationObserver);
};

}  // namespace

namespace ash {

ShelfViewTestAPI::ShelfViewTestAPI(ShelfView* shelf_view)
    : shelf_view_(shelf_view) {}

ShelfViewTestAPI::~ShelfViewTestAPI() = default;

int ShelfViewTestAPI::GetButtonCount() {
  return shelf_view_->view_model_->view_size();
}

ShelfButton* ShelfViewTestAPI::GetButton(int index) {
  // App list and back button are not ShelfButtons.
  if (shelf_view_->model_->items()[index].type == ash::TYPE_APP_LIST ||
      shelf_view_->model_->items()[index].type == ash::TYPE_BACK_BUTTON)
    return nullptr;

  return static_cast<ShelfButton*>(GetViewAt(index));
}

views::View* ShelfViewTestAPI::GetViewAt(int index) {
  return shelf_view_->view_model_->view_at(index);
}

int ShelfViewTestAPI::GetFirstVisibleIndex() {
  return shelf_view_->first_visible_index_;
}

int ShelfViewTestAPI::GetLastVisibleIndex() {
  return shelf_view_->last_visible_index_;
}

bool ShelfViewTestAPI::IsOverflowButtonVisible() {
  return shelf_view_->overflow_button_->visible();
}

void ShelfViewTestAPI::ShowOverflowBubble() {
  DCHECK(!shelf_view_->IsShowingOverflowBubble());
  shelf_view_->ToggleOverflowBubble();
}

void ShelfViewTestAPI::HideOverflowBubble() {
  DCHECK(shelf_view_->IsShowingOverflowBubble());
  shelf_view_->ToggleOverflowBubble();
}

bool ShelfViewTestAPI::IsShowingOverflowBubble() const {
  return shelf_view_->IsShowingOverflowBubble();
}

const gfx::Rect& ShelfViewTestAPI::GetBoundsByIndex(int index) {
  return shelf_view_->view_model_->view_at(index)->bounds();
}

const gfx::Rect& ShelfViewTestAPI::GetIdealBoundsByIndex(int index) {
  return shelf_view_->view_model_->ideal_bounds(index);
}

int ShelfViewTestAPI::GetAnimationDuration() const {
  DCHECK(shelf_view_->bounds_animator_);
  return shelf_view_->bounds_animator_->GetAnimationDuration();
}

void ShelfViewTestAPI::SetAnimationDuration(int duration_ms) {
  shelf_view_->bounds_animator_->SetAnimationDuration(duration_ms);
}

void ShelfViewTestAPI::RunMessageLoopUntilAnimationsDone() {
  if (!shelf_view_->bounds_animator_->IsAnimating())
    return;

  std::unique_ptr<TestAPIAnimationObserver> observer(
      new TestAPIAnimationObserver());
  shelf_view_->bounds_animator_->AddObserver(observer.get());

  // This nested loop will quit when TestAPIAnimationObserver's
  // OnBoundsAnimatorDone is called.
  base::RunLoop().Run();

  shelf_view_->bounds_animator_->RemoveObserver(observer.get());
}

gfx::Rect ShelfViewTestAPI::GetMenuAnchorRect(const views::View* source,
                                              const gfx::Point& location,
                                              ui::MenuSourceType source_type,
                                              bool context_menu) const {
  return shelf_view_->GetMenuAnchorRect(source, location, source_type,
                                        context_menu);
}

bool ShelfViewTestAPI::CloseMenu() {
  if (!shelf_view_->launcher_menu_runner_)
    return false;

  shelf_view_->launcher_menu_runner_->Cancel();
  return true;
}

OverflowBubble* ShelfViewTestAPI::overflow_bubble() {
  return shelf_view_->overflow_bubble_.get();
}

OverflowButton* ShelfViewTestAPI::overflow_button() const {
  return shelf_view_->overflow_button_;
}

ShelfTooltipManager* ShelfViewTestAPI::tooltip_manager() {
  return &shelf_view_->tooltip_;
}

int ShelfViewTestAPI::GetMinimumDragDistance() const {
  return ShelfView::kMinimumDragDistance;
}

bool ShelfViewTestAPI::SameDragType(ShelfItemType typea,
                                    ShelfItemType typeb) const {
  return shelf_view_->SameDragType(typea, typeb);
}

gfx::Rect ShelfViewTestAPI::GetBoundsForDragInsertInScreen() {
  return shelf_view_->GetBoundsForDragInsertInScreen();
}

bool ShelfViewTestAPI::IsRippedOffFromShelf() {
  return shelf_view_->dragged_off_shelf_;
}

bool ShelfViewTestAPI::DraggedItemToAnotherShelf() {
  return shelf_view_->dragged_to_another_shelf_;
}

ShelfButtonPressedMetricTracker*
ShelfViewTestAPI::shelf_button_pressed_metric_tracker() {
  return &(shelf_view_->shelf_button_pressed_metric_tracker_);
}

}  // namespace ash
