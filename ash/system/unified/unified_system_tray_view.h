// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_UNIFIED_UNIFIED_SYSTEM_TRAY_VIEW_H_
#define ASH_SYSTEM_UNIFIED_UNIFIED_SYSTEM_TRAY_VIEW_H_

#include "ui/views/view.h"

namespace ash {

class FeaturePodButton;
class FeaturePodsContainerView;
class TopShortcutsView;
class UnifiedMessageCenterView;
class UnifiedSystemInfoView;
class UnifiedSystemTrayController;

// Container view of slider views. If SetExpandedAmount() is called with 1.0,
// the behavior is same as vertiacal BoxLayout, but otherwise it shows
// intermediate state during animation.
class UnifiedSlidersContainerView : public views::View {
 public:
  explicit UnifiedSlidersContainerView(bool initially_expanded);
  ~UnifiedSlidersContainerView() override;

  // Change the expanded state. 0.0 if collapsed, and 1.0 if expanded.
  // Otherwise, it shows intermediate state.
  void SetExpandedAmount(double expanded_amount);

  // views::View:
  void Layout() override;
  gfx::Size CalculatePreferredSize() const override;

 private:
  // Update opacity of each child slider views based on |expanded_amount_|.
  void UpdateOpacity();

  double expanded_amount_;

  DISALLOW_COPY_AND_ASSIGN(UnifiedSlidersContainerView);
};

// View class of the main bubble in UnifiedSystemTray.
class UnifiedSystemTrayView : public views::View {
 public:
  UnifiedSystemTrayView(UnifiedSystemTrayController* controller,
                        bool initially_expanded);
  ~UnifiedSystemTrayView() override;

  // Set the maximum height that the view can take.
  void SetMaxHeight(int max_height);

  // Add feature pod button to |feature_pods_|.
  void AddFeaturePodButton(FeaturePodButton* button);

  // Add slider view.
  void AddSliderView(views::View* slider_view);

  // Hide the main view and show the given |detailed_view|.
  void SetDetailedView(views::View* detailed_view);

  // Change the expanded state. 0.0 if collapsed, and 1.0 if expanded.
  // Otherwise, it shows intermediate state.
  void SetExpandedAmount(double expanded_amount);

  // views::View:
  void OnGestureEvent(ui::GestureEvent* event) override;
  void ChildPreferredSizeChanged(views::View* child) override;

 private:
  // Unowned.
  UnifiedSystemTrayController* controller_;

  // Owned by views hierarchy.
  UnifiedMessageCenterView* message_center_view_;
  TopShortcutsView* top_shortcuts_view_;
  FeaturePodsContainerView* feature_pods_container_;
  UnifiedSlidersContainerView* sliders_container_;
  UnifiedSystemInfoView* system_info_view_;
  views::View* system_tray_container_;
  views::View* detailed_view_container_;

  const std::unique_ptr<ui::EventHandler> interacted_by_tap_recorder_;

  DISALLOW_COPY_AND_ASSIGN(UnifiedSystemTrayView);
};

}  // namespace ash

#endif  // ASH_SYSTEM_UNIFIED_UNIFIED_SYSTEM_TRAY_VIEW_H_
