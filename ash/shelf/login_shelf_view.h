// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SHELF_LOGIN_SHELF_VIEW_H_
#define ASH_SHELF_LOGIN_SHELF_VIEW_H_

#include "ash/ash_export.h"
#include "ash/lock_screen_action/lock_screen_action_background_observer.h"
#include "ash/shutdown_controller.h"
#include "ash/tray_action/tray_action_observer.h"
#include "base/scoped_observer.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/view.h"

namespace gfx {
class ImageSkia;
}

namespace session_manager {
enum class SessionState;
}

namespace ash {

class LockScreenActionBackgroundController;
enum class LockScreenActionBackgroundState;
class TrayAction;

// LoginShelfView contains the shelf buttons visible outside of an active user
// session. ShelfView and LoginShelfView should never be shown together.
class ASH_EXPORT LoginShelfView : public views::View,
                                  public views::ButtonListener,
                                  public TrayActionObserver,
                                  public LockScreenActionBackgroundObserver,
                                  public ShutdownController::Observer {
 public:
  enum ButtonId {
    kShutdown = 1,   // Shut down the device.
    kRestart,        // Restart the device.
    kSignOut,        // Sign out the active user session.
    kCloseNote,      // Close the lock screen note.
    kCancel,         // Cancel multiple user sign-in.
    kBrowseAsGuest,  // Use in guest mode.
    kAddUser,        // Add a new user.
    kShowWebUiLogin  // Show webui login.
  };

  explicit LoginShelfView(
      LockScreenActionBackgroundController* lock_screen_action_background);
  ~LoginShelfView() override;

  // ShelfWidget observes SessionController for higher-level UI changes and
  // then notifies LoginShelfView to update its own UI.
  void UpdateAfterSessionStateChange(session_manager::SessionState state);

  // views::View:
  const char* GetClassName() const override;
  void OnFocus() override;
  void AboutToRequestFocusFromTabTraversal(bool reverse) override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;

  // views::ButtonListener:
  void ButtonPressed(views::Button* sender, const ui::Event& event) override;

 protected:
  // TrayActionObserver:
  void OnLockScreenNoteStateChanged(mojom::TrayActionState state) override;

  // LockScreenActionBackgroundObserver:
  void OnLockScreenActionBackgroundStateChanged(
      LockScreenActionBackgroundState state) override;

  // ShutdownController::Observer:
  void OnShutdownPolicyChanged(bool reboot_on_shutdown) override;

 private:
  bool LockScreenActionBackgroundAnimating() const;

  // Updates the visibility of buttons based on state changes, e.g. shutdown
  // policy updates, session state changes etc.
  void UpdateUi();

  LockScreenActionBackgroundController* lock_screen_action_background_;

  ScopedObserver<TrayAction, TrayActionObserver> tray_action_observer_;

  ScopedObserver<LockScreenActionBackgroundController,
                 LockScreenActionBackgroundObserver>
      lock_screen_action_background_observer_;

  ScopedObserver<ShutdownController, ShutdownController::Observer>
      shutdown_controller_observer_;

  DISALLOW_COPY_AND_ASSIGN(LoginShelfView);
};

}  // namespace ash

#endif  // ASH_SHELF_LOGIN_SHELF_VIEW_H_
