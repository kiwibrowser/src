// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/unified/feature_pods_container_view.h"

#include "ash/system/tray/tray_constants.h"
#include "ash/system/unified/feature_pod_button.h"

namespace ash {

FeaturePodsContainerView::FeaturePodsContainerView(bool initially_expanded)
    : expanded_amount_(initially_expanded ? 1.0 : 0.0) {}

FeaturePodsContainerView::~FeaturePodsContainerView() = default;

gfx::Size FeaturePodsContainerView::CalculatePreferredSize() const {
  const int collapsed_height = 2 * kUnifiedFeaturePodCollapsedVerticalPadding +
                               kUnifiedFeaturePodCollapsedSize.height();

  int visible_count = 0;
  for (int i = 0; i < child_count(); ++i) {
    if (static_cast<const FeaturePodButton*>(child_at(i))->visible_preferred())
      ++visible_count;
  }

  // floor(visible_count / kUnifiedFeaturePodItemsInRow)
  int number_of_lines = (visible_count + kUnifiedFeaturePodItemsInRow - 1) /
                        kUnifiedFeaturePodItemsInRow;
  const int expanded_height =
      kUnifiedFeaturePodVerticalPadding +
      (kUnifiedFeaturePodVerticalPadding + kUnifiedFeaturePodSize.height()) *
          number_of_lines;

  return gfx::Size(
      kTrayMenuWidth,
      static_cast<int>(collapsed_height * (1.0 - expanded_amount_) +
                       expanded_height * expanded_amount_));
}

void FeaturePodsContainerView::SetExpandedAmount(double expanded_amount) {
  DCHECK(0.0 <= expanded_amount && expanded_amount <= 1.0);
  if (expanded_amount_ == expanded_amount)
    return;
  expanded_amount_ = expanded_amount;

  PreferredSizeChanged();

  for (int i = 0; i < child_count(); ++i) {
    auto* child = static_cast<FeaturePodButton*>(child_at(i));
    child->SetExpandedAmount(expanded_amount_);
  }
  UpdateChildVisibility();
  // We have to call Layout() explicitly here.
  Layout();
}

void FeaturePodsContainerView::ChildVisibilityChanged(View* child) {
  // ChildVisibilityChanged can change child visibility in
  // UpdateChildVisibility(), so we have to prevent this.
  if (changing_visibility_)
    return;

  UpdateChildVisibility();
  Layout();
  SchedulePaint();
}

void FeaturePodsContainerView::Layout() {
  UpdateCollapsedSidePadding();

  int visible_count = 0;
  for (int i = 0; i < child_count(); ++i) {
    views::View* child = child_at(i);
    if (!child->visible())
      continue;

    child->SetBoundsRect(gfx::Rect(GetButtonPosition(visible_count++),
                                   expanded_amount_ > 0.0
                                       ? kUnifiedFeaturePodSize
                                       : kUnifiedFeaturePodCollapsedSize));
    child->Layout();
  }
}

void FeaturePodsContainerView::UpdateChildVisibility() {
  DCHECK(!changing_visibility_);
  changing_visibility_ = true;

  int visible_count = 0;
  for (int i = 0; i < child_count(); ++i) {
    auto* child = static_cast<FeaturePodButton*>(child_at(i));
    bool visible = child->visible_preferred() &&
                   (expanded_amount_ > 0.0 ||
                    visible_count < kUnifiedFeaturePodMaxItemsInCollapsed);
    child->SetVisibleByContainer(visible);
    if (visible)
      ++visible_count;
  }

  changing_visibility_ = false;
}

gfx::Point FeaturePodsContainerView::GetButtonPosition(
    int visible_index) const {
  int row = visible_index / kUnifiedFeaturePodItemsInRow;
  int column = visible_index % kUnifiedFeaturePodItemsInRow;
  int x = kUnifiedFeaturePodHorizontalSidePadding +
          (kUnifiedFeaturePodSize.width() +
           kUnifiedFeaturePodHorizontalMiddlePadding) *
              column;
  int y =
      kUnifiedFeaturePodVerticalPadding +
      (kUnifiedFeaturePodSize.height() + kUnifiedFeaturePodVerticalPadding) *
          row;

  // When fully expanded, or below the second row, always return the same
  // position.
  if (expanded_amount_ == 1.0 || row > 2)
    return gfx::Point(x, y);

  int collapsed_x =
      collapsed_side_padding_ + (kUnifiedFeaturePodCollapsedSize.width() +
                                 kUnifiedFeaturePodCollapsedHorizontalPadding) *
                                    visible_index;
  int collapsed_y = kUnifiedFeaturePodCollapsedVerticalPadding;

  // When fully collapsed, just return the collapsed position.
  if (expanded_amount_ == 0.0)
    return gfx::Point(collapsed_x, collapsed_y);

  // Button width is different between expanded and collapsed states.
  // During the transition, expanded width is used, so it should be adjusted.
  collapsed_x -= (kUnifiedFeaturePodSize.width() -
                  kUnifiedFeaturePodCollapsedSize.width()) /
                 2;

  return gfx::Point(
      x * expanded_amount_ + collapsed_x * (1.0 - expanded_amount_),
      y * expanded_amount_ + collapsed_y * (1.0 - expanded_amount_));
}

void FeaturePodsContainerView::UpdateCollapsedSidePadding() {
  int visible_count = 0;
  for (int i = 0; i < child_count(); ++i) {
    if (static_cast<const FeaturePodButton*>(child_at(i))->visible_preferred())
      ++visible_count;
  }

  visible_count =
      std::min(visible_count, kUnifiedFeaturePodMaxItemsInCollapsed);

  int contents_width =
      visible_count * kUnifiedFeaturePodCollapsedSize.width() +
      (visible_count - 1) * kUnifiedFeaturePodCollapsedHorizontalPadding;

  collapsed_side_padding_ = (kTrayMenuWidth - contents_width) / 2;
  DCHECK(collapsed_side_padding_ > 0);
}

}  // namespace ash
