// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_UNIFIED_UNIFIED_SYSTEM_TRAY_CONTROLLER_H_
#define ASH_SYSTEM_UNIFIED_UNIFIED_SYSTEM_TRAY_CONTROLLER_H_

#include <memory>
#include <vector>

#include "ash/ash_export.h"
#include "base/macros.h"
#include "ui/gfx/animation/animation_delegate.h"
#include "ui/gfx/geometry/point.h"

namespace gfx {
class SlideAnimation;
}  // namespace gfx

namespace ash {

class DetailedViewController;
class FeaturePodControllerBase;
class SystemTray;
class SystemTrayItem;
class UnifiedBrightnessSliderController;
class UnifiedVolumeSliderController;
class UnifiedSystemTrayModel;
class UnifiedSystemTrayView;

// Controller class of UnifiedSystemTrayView. Handles events of the view.
class ASH_EXPORT UnifiedSystemTrayController : public gfx::AnimationDelegate {
 public:
  // |system_tray| is used to show detailed views which are still not
  // implemented on UnifiedSystemTray.
  UnifiedSystemTrayController(UnifiedSystemTrayModel* model,
                              SystemTray* system_tray);
  ~UnifiedSystemTrayController() override;

  // Create the view. The created view is unowned.
  UnifiedSystemTrayView* CreateView();

  // Switch the active user to |user_index|. Called from the view.
  void HandleUserSwitch(int user_index);
  // Show multi profile login UI. Called from the view.
  void HandleAddUserAction();
  // Sign out from the current user. Called from the view.
  void HandleSignOutAction();
  // Show lock screen which asks the user password. Called from the view.
  void HandleLockAction();
  // Show WebUI settings. Called from the view.
  void HandleSettingsAction();
  // Shutdown the computer. Called from the view.
  void HandlePowerAction();
  // Toggle expanded state of UnifiedSystemTrayView. Called from the view.
  void ToggleExpanded();

  // Handle finger dragging and expand/collapse the view. Called from view.
  void BeginDrag(const gfx::Point& location);
  void UpdateDrag(const gfx::Point& location);
  void EndDrag(const gfx::Point& location);

  // Show user selector popup widget. Called from the view.
  void ShowUserChooserWidget();
  // Show the detailed view of network. Called from the view.
  void ShowNetworkDetailedView();
  // Show the detailed view of bluetooth. Called from the view.
  void ShowBluetoothDetailedView();
  // Show the detailed view of cast. Called from the view.
  void ShowCastDetailedView();
  // Show the detailed view of accessibility. Called from the view.
  void ShowAccessibilityDetailedView();
  // Show the detailed view of VPN. Called from the view.
  void ShowVPNDetailedView();
  // Show the detailed view of IME. Called from the view.
  void ShowIMEDetailedView();

  // If you want to add a new detailed view, add here.

  // gfx::AnimationDelegate:
  void AnimationEnded(const gfx::Animation* animation) override;
  void AnimationProgressed(const gfx::Animation* animation) override;
  void AnimationCanceled(const gfx::Animation* animation) override;

  UnifiedSystemTrayModel* model() { return model_; }

 private:
  friend class UnifiedSystemTrayControllerTest;

  // How the expanded state is toggled. The enum is used to back an UMA
  // histogram and should be treated as append-only.
  enum ToggleExpandedType {
    TOGGLE_EXPANDED_TYPE_BY_BUTTON = 0,
    TOGGLE_EXPANDED_TYPE_BY_GESTURE,
    TOGGLE_EXPANDED_TYPE_COUNT
  };

  // Initialize feature pod controllers and their views.
  // If you want to add a new feature pod item, you have to add here.
  void InitFeaturePods();

  // Add the feature pod controller and its view.
  void AddFeaturePodItem(std::unique_ptr<FeaturePodControllerBase> controller);

  // Show the detailed view.
  void ShowDetailedView(std::unique_ptr<DetailedViewController> controller);

  // Show detailed view of SystemTrayItem.
  // TODO(tetsui): Remove when Unified's own DetailedViewControllers are
  // implemented.
  void ShowSystemTrayItemDetailedView(SystemTrayItem* system_tray_item);

  // Update how much the view is expanded based on |animation_|.
  void UpdateExpandedAmount();

  // Return touch drag amount between 0.0 and 1.0. If expanding, it increases
  // towards 1.0. If collapsing, it decreases towards 0.0. If the view is
  // dragged to the same direction as the current state, it does not change the
  // value. For example, if the view is expanded and it's dragged to the top, it
  // keeps returning 1.0.
  double GetDragExpandedAmount(const gfx::Point& location) const;

  // Model that stores UI specific variables. Unowned.
  UnifiedSystemTrayModel* const model_;

  // Only used to show detailed views which are still not implemented on
  // UnifiedSystemTray. Unowned.
  // TODO(tetsui): Remove reference to |system_tray|.
  SystemTray* const system_tray_;

  // Unowned. Owned by Views hierarchy.
  UnifiedSystemTrayView* unified_view_ = nullptr;

  // The controller of the current detailed view. If the main view is shown,
  // it's null. Owned.
  std::unique_ptr<DetailedViewController> detailed_view_controller_;

  // Controllers of feature pod buttons. Owned by this.
  std::vector<std::unique_ptr<FeaturePodControllerBase>>
      feature_pod_controllers_;

  // Controller of volume slider. Owned.
  std::unique_ptr<UnifiedVolumeSliderController> volume_slider_controller_;

  // Controller of brightness slider. Owned.
  std::unique_ptr<UnifiedBrightnessSliderController>
      brightness_slider_controller_;

  // If the previous state is expanded or not. Only valid during dragging (from
  // BeginDrag to EndDrag).
  bool was_expanded_ = true;

  // The last |location| passed to BeginDrag(). Only valid during dragging.
  gfx::Point drag_init_point_;

  // Animation between expanded and collapsed states.
  std::unique_ptr<gfx::SlideAnimation> animation_;

  DISALLOW_COPY_AND_ASSIGN(UnifiedSystemTrayController);
};

}  // namespace ash

#endif  // ASH_SYSTEM_UNIFIED_UNIFIED_SYSTEM_TRAY_CONTROLLER_H_
