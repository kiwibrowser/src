// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_IME_TRAY_IME_CHROMEOS_H_
#define ASH_SYSTEM_IME_TRAY_IME_CHROMEOS_H_

#include <stddef.h>

#include "ash/accessibility/accessibility_observer.h"
#include "ash/public/interfaces/ime_info.mojom.h"
#include "ash/system/ime/ime_observer.h"
#include "ash/system/ime_menu/ime_list_view.h"
#include "ash/system/tray/system_tray_item.h"
#include "ash/system/virtual_keyboard/virtual_keyboard_observer.h"
#include "base/macros.h"

namespace ash {

namespace tray {
class IMEDefaultView;
class IMEDetailedView;
}

class ImeController;
class TrayItemView;
class DetailedViewDelegate;

// Controller for IME options in the system menu. Note this is separate from
// the "opt-in" IME menu which can appear as a button in the system tray
// area; that is controlled by ImeMenuTray.
class ASH_EXPORT TrayIME : public SystemTrayItem,
                           public IMEObserver,
                           public AccessibilityObserver,
                           public VirtualKeyboardObserver {
 public:
  explicit TrayIME(SystemTray* system_tray);
  ~TrayIME() override;

  // Overridden from VirtualKeyboardObserver.
  void OnKeyboardSuppressionChanged(bool suppressed) override;

  // Overridden from AccessibilityObserver:
  void OnAccessibilityStatusChanged() override;

 private:
  friend class TrayIMETest;

  // Repopulates the DefaultView and DetailedView.
  void Update();
  // Updates the System Tray label.
  void UpdateTrayLabel(const mojom::ImeInfo& info, size_t count);
  // Returns whether the virtual keyboard toggle should be shown in the
  // detailed view.
  bool ShouldShowKeyboardToggle();
  // Returns the appropriate label for the detailed view.
  base::string16 GetDefaultViewLabel(bool show_ime_label);

  // Overridden from SystemTrayItem.
  views::View* CreateTrayView(LoginStatus status) override;
  views::View* CreateDefaultView(LoginStatus status) override;
  views::View* CreateDetailedView(LoginStatus status) override;
  void OnTrayViewDestroyed() override;
  void OnDefaultViewDestroyed() override;
  void OnDetailedViewDestroyed() override;

  // Overridden from IMEObserver.
  void OnIMERefresh() override;
  void OnIMEMenuActivationChanged(bool is_active) override;

  // Returns true if input methods are managed by policy.
  bool IsIMEManaged();

  // Whether the default view should be shown.
  bool ShouldDefaultViewBeVisible();

  // Decides if a tray icon should be shown.
  bool ShouldShowImeTrayItem(size_t ime_count);
  // Mandates behavior for the single IME case for the detailed IME list
  // sub-view.
  ImeListView::SingleImeBehavior GetSingleImeBehavior();

  // Returns the icon used when the IME is managed.
  views::View* GetControlledSettingIconForTesting();

  ImeController* ime_controller_;
  TrayItemView* tray_label_;
  tray::IMEDefaultView* default_;
  tray::IMEDetailedView* detailed_;

  // Whether the virtual keyboard is suppressed.
  bool keyboard_suppressed_;

  // Whether the IME label and tray items should be visible.
  bool is_visible_;

  const std::unique_ptr<DetailedViewDelegate> detailed_view_delegate_;

  DISALLOW_COPY_AND_ASSIGN(TrayIME);
};

}  // namespace ash

#endif  // ASH_SYSTEM_IME_TRAY_IME_CHROMEOS_H_
