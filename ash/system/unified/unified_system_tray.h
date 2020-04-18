// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_UNIFIED_UNIFIED_SYSTEM_TRAY_H_
#define ASH_SYSTEM_UNIFIED_UNIFIED_SYSTEM_TRAY_H_

#include <memory>

#include "ash/system/tray/tray_background_view.h"

namespace ash {

class NotificationCounterView;
class QuietModeView;
class UnifiedSystemTrayBubble;
class UnifiedSystemTrayModel;

// UnifiedSystemTray is system menu of Chromium OS, which is typically
// accessible from the button on the right bottom of the screen (Status Area).
// The button shows multiple icons on it to indicate system status.
// UnifiedSystemTrayBubble is the actual menu bubble shown on top of it when the
// button is clicked.
//
// UnifiedSystemTray is the view class of that button. It creates and owns
// UnifiedSystemTrayBubble when it is clicked.
//
// UnifiedSystemTray is alternative implementation of SystemTray that is going
// to replace the original one. Eventually, SystemTray will be removed.
// During the transition, we hide UnifiedSystemTray button and forward
// PerformAction() from SystemTray button, as we still don't have implementation
// of status icons on the UnifiedSystemTray button.
class UnifiedSystemTray : public TrayBackgroundView {
 public:
  explicit UnifiedSystemTray(Shelf* shelf);
  ~UnifiedSystemTray() override;

  // True if the bubble is shown.
  bool IsBubbleShown() const;

  // Return the bounds of the bubble in the screen.
  gfx::Rect GetBubbleBoundsInScreen() const;

  // TrayBackgroundView:
  bool PerformAction(const ui::Event& event) override;
  void ShowBubble(bool show_by_click) override;
  void CloseBubble() override;
  base::string16 GetAccessibleNameForTray() override;
  void HideBubbleWithView(const views::TrayBubbleView* bubble_view) override;
  void ClickedOutsideBubble() override;

  UnifiedSystemTrayModel* model() { return model_.get(); }

 private:
  // Private class implements message_center::UiDelegate.
  class UiDelegate;

  // Private class implements TrayNetworkStateObserver::Delegate.
  class NetworkStateDelegate;

  // Forwarded from UiDelegate.
  void ShowBubbleInternal(bool show_by_click);
  void HideBubbleInternal();
  void UpdateNotificationInternal();

  const std::unique_ptr<UiDelegate> ui_delegate_;

  std::unique_ptr<NetworkStateDelegate> network_state_delegate_;

  std::unique_ptr<UnifiedSystemTrayBubble> bubble_;

  // Model class that stores UnifiedSystemTray's UI specific variables.
  const std::unique_ptr<UnifiedSystemTrayModel> model_;

  NotificationCounterView* const notification_counter_item_;
  QuietModeView* const quiet_mode_view_;

  DISALLOW_COPY_AND_ASSIGN(UnifiedSystemTray);
};

}  // namespace ash

#endif  // ASH_SYSTEM_UNIFIED_UNIFIED_SYSTEM_TRAY_H_
