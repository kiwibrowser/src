// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_ROTATION_TRAY_ROTATION_LOCK_H_
#define ASH_SYSTEM_ROTATION_TRAY_ROTATION_LOCK_H_

#include "ash/display/screen_orientation_controller.h"
#include "ash/system/tray/tray_image_item.h"
#include "ash/wm/tablet_mode/tablet_mode_observer.h"
#include "base/macros.h"

namespace ash {

// TrayRotationLock is a provider of views for the SystemTray. Both a tray view
// and a default view are provided. Each view indicates the current state of
// the rotation lock for the display which it appears on. The default view can
// be interacted with, it toggles the state of the rotation lock.
// TrayRotationLock is only available on the primary display.
class ASH_EXPORT TrayRotationLock
    : public TrayImageItem,
      public ScreenOrientationController::Observer,
      public TabletModeObserver {
 public:
  explicit TrayRotationLock(SystemTray* system_tray);
  ~TrayRotationLock() override;

  // ScreenOrientationController::Observer:
  void OnUserRotationLockChanged() override;

  // SystemTrayItem:
  views::View* CreateDefaultView(LoginStatus status) override;

  // TabletModeObserver:
  void OnTabletModeStarted() override;
  void OnTabletModeEnded() override;

  // TrayImageItem:
  void OnTrayViewDestroyed() override;

 protected:
  // TrayImageItem:
  bool GetInitialVisibility() override;

 private:
  friend class TrayRotationLockTest;

  // Update tray image based on whether user rotation lock is enabled.
  void UpdateTrayImage();

  // True if |on_primary_display_|, tablet mode is enabled.
  bool ShouldBeVisible();

  // True if this is owned by a SystemTray on the primary display.
  bool OnPrimaryDisplay() const;

  // Removes TrayRotationLock as a ScreenOrientationController::Observer if
  // currently observing.
  void StopObservingRotation();

  DISALLOW_COPY_AND_ASSIGN(TrayRotationLock);
};

}  // namespace ash

#endif  // ASH_SYSTEM_ROTATION_TRAY_ROTATION_LOCK_H_
