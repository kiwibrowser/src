// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_TRAY_SYSTEM_TRAY_ITEM_H_
#define ASH_SYSTEM_TRAY_SYSTEM_TRAY_ITEM_H_

#include <memory>

#include "ash/ash_export.h"
#include "ash/login_status.h"
#include "ash/public/cpp/shelf_types.h"
#include "base/macros.h"
#include "base/timer/timer.h"

namespace views {
class View;
}

namespace ash {
class SystemTray;
class SystemTrayView;
class TrayItemView;

// Controller for an item in the system tray. Each item can create these views:
// Tray view - The icon in the status area in the shelf.
// Default view - The row in the top-level menu.
// Detailed view - The submenu shown when the top-level menu row is clicked.
class ASH_EXPORT SystemTrayItem {
 public:
  // The different types of SystemTrayItems.
  //
  // NOTE: These values are used for UMA metrics so do NOT re-order this enum
  // and only insert items before the COUNT item.
  enum UmaType {
    // SystemTrayItem's with this type are not recorded in the histogram.
    UMA_NOT_RECORDED = 0,
    // Used for testing purposes only.
    UMA_TEST = 1,
    UMA_ACCESSIBILITY = 2,
    UMA_AUDIO = 3,
    UMA_BLUETOOTH = 4,
    UMA_CAPS_LOCK = 5,
    UMA_CAST = 6,
    UMA_DATE = 7,
    UMA_DISPLAY = 8,
    UMA_DISPLAY_BRIGHTNESS = 9,
    UMA_ENTERPRISE = 10,
    UMA_IME = 11,
    UMA_MULTI_PROFILE_MEDIA = 12,
    UMA_NETWORK = 13,
    UMA_SETTINGS = 14,
    UMA_UPDATE = 15,
    UMA_POWER = 16,
    UMA_ROTATION_LOCK = 17,
    UMA_SCREEN_CAPTURE = 18,
    UMA_SCREEN_SHARE = 19,
    UMA_SESSION_LENGTH_LIMIT = 20,
    UMA_SMS = 21,
    UMA_SUPERVISED_USER = 22,
    UMA_TRACING = 23,
    UMA_USER = 24,
    UMA_VPN = 25,
    UMA_NIGHT_LIGHT = 26,
    UMA_COUNT = 27,
  };

  SystemTrayItem(SystemTray* system_tray, UmaType type);
  virtual ~SystemTrayItem();

  // Create* functions may return NULL if nothing should be displayed for the
  // type of view. The default implementations return NULL.

  // Returns a view to be displayed in the system tray. If this returns NULL,
  // then this item is not displayed in the tray.
  // NOTE: The returned view should almost always be a TrayItemView, which
  // automatically resizes the widget when the size of the view changes, and
  // adds animation when the visibility of the view changes. If a view wants to
  // avoid this behavior, then it should not be a TrayItemView.
  virtual views::View* CreateTrayView(LoginStatus status);

  // Returns a view for the item to be displayed in the list. This view can be
  // displayed with a number of other tray items, so this should not be too
  // big.
  virtual views::View* CreateDefaultView(LoginStatus status);

  // Returns a detailed view for the item. This view is displayed standalone.
  virtual views::View* CreateDetailedView(LoginStatus status);

  // These functions are called when the corresponding view item is about to be
  // removed. An item should do appropriate cleanup in these functions.
  // The default implementation does nothing.
  virtual void OnTrayViewDestroyed();
  virtual void OnDefaultViewDestroyed();
  virtual void OnDetailedViewDestroyed();

  // Updates the tray view (if applicable) when the user's login status changes.
  // It is not necessary the update the default or detailed view, since the
  // default/detailed popup is closed when login status changes. The default
  // implementation does nothing.
  virtual void UpdateAfterLoginStatusChange(LoginStatus status);

  // Updates the tray view (if applicable) when shelf's alignment changes.
  // The default implementation does nothing.
  virtual void UpdateAfterShelfAlignmentChange();

  // Shows the detailed view for this item. If the main popup for the tray is
  // currently visible, then making this call would use the existing window to
  // display the detailed item. The detailed item will inherit the bounds of the
  // existing window.
  //
  // In Material Design the actual transition is intentionally delayed to allow
  // the user to perceive the ink drop animation on the clicked target.
  void TransitionDetailedView();

  // Pops up the detailed view for this item. An item can request to show its
  // detailed view using this function (e.g. from an observer callback when
  // something, e.g. volume, network availability etc. changes). If
  // |for_seconds| is non-zero, then the popup is closed after the specified
  // time.
  void ShowDetailedView(int for_seconds);

  // Continue showing the currently-shown detailed view, if any, for
  // |for_seconds| seconds.  The caller is responsible for checking that the
  // currently-shown view is for this item.
  void SetDetailedViewCloseDelay(int for_seconds);

  // Hides the detailed view for this item.
  void HideDetailedView();

  // Returns true if this item needs to force the shelf to be visible when
  // the shelf is in the auto-hide state. Default is true.
  virtual bool ShouldShowShelf() const;

  // Returns true if the bubble of UnifiedSystemTray is shown.
  bool IsUnifiedBubbleShown();

  // Returns the view that should be focused when leaving a detailed view to the
  // default view. Returns nullptr if the view that should be focused is just
  // the default view.
  virtual views::View* GetItemToRestoreFocusTo();

  // Returns the system tray that this item belongs to.
  SystemTray* system_tray() const { return system_tray_; }

  bool restore_focus() const { return restore_focus_; }
  void set_restore_focus(bool restore_focus) { restore_focus_ = restore_focus; }

 private:
  // Accesses uma_type().
  friend class SystemTrayView;

  UmaType uma_type() const { return uma_type_; }

  SystemTray* system_tray_;
  UmaType uma_type_;
  bool restore_focus_;

  // Used to delay the transition to the detailed view.
  base::OneShotTimer transition_delay_timer_;

  DISALLOW_COPY_AND_ASSIGN(SystemTrayItem);
};

}  // namespace ash

#endif  // ASH_SYSTEM_TRAY_SYSTEM_TRAY_ITEM_H_
