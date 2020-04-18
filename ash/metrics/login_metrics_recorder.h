// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_METRICS_LOGIN_METRICS_RECORDER_H_
#define ASH_METRICS_LOGIN_METRICS_RECORDER_H_

#include "ash/ash_export.h"
#include "base/macros.h"

namespace ash {

// A metrics recorder that records login authentication related metrics.
// This keeps track of the last authentication method we used and records
// switching between different authentication methods.
// This is tied to UserMetricsRecorder lifetime.
class ASH_EXPORT LoginMetricsRecorder {
 public:
  // Authentication method to unlock the screen. This enum is used to back an
  // UMA histogram and new values should be inserted immediately above
  // kMethodCount.
  enum class AuthMethod {
    kPassword = 0,
    kPin,
    kSmartlock,
    kMethodCount,
  };

  // The type of switching between auth methods. This enum is used to back an
  // UMA histogram and new values should be inserted immediately above
  // kSwitchTypeCount.
  enum class AuthMethodSwitchType {
    kPasswordToPin = 0,
    kPasswordToSmartlock,
    kPinToPassword,
    kPinToSmartlock,
    kSmartlockToPin,
    kSmartlockToPassword,
    kSwitchTypeCount,
  };

  // User clicks target on the lock screen. This enum is used to back an UMA
  // histogram and new values should be inserted immediately above kTargetCount.
  enum class LockScreenUserClickTarget {
    kShutDownButton = 0,
    kRestartButton,
    kSignOutButton,
    kCloseNoteButton,
    kSystemTray,
    kVirtualKeyboardTray,
    kImeTray,
    kNotificationTray,
    kLockScreenNoteActionButton,
    kTargetCount,
  };

  // User clicks target on the login screen. This enum is used to back an UMA
  // histogram and new values should be inserted immediately above kTargetCount.
  enum class LoginScreenUserClickTarget {
    kShutDownButton = 0,
    kRestartButton,
    kBrowseAsGuestButton,
    kAddUserButton,
    kSystemTray,
    kVirtualKeyboardTray,
    kImeTray,
    kTargetCount,
  };

  // Helper enumeration for tray related user click targets on login and lock
  // screens. Values are translated to UMA histogram enums.
  enum class TrayClickTarget {
    kSystemTray,
    kVirtualKeyboardTray,
    kImeTray,
    kNotificationTray,
    kTrayActionNoteButton,
    kTargetCount,
  };

  // Helper enumeration for shelf buttons user click targets on login and lock
  // screens. Values are translated to UMA histogram enums.
  enum class ShelfButtonClickTarget {
    kShutDownButton,
    kRestartButton,
    kSignOutButton,
    kBrowseAsGuestButton,
    kAddUserButton,
    kCloseNoteButton,
    kCancelButton,
    kTargetCount,
  };

  LoginMetricsRecorder();
  ~LoginMetricsRecorder();

  // Called when user attempts authentication using AuthMethod |type|.
  void SetAuthMethod(AuthMethod type);

  // Called when lock state changed.
  void Reset();

  // Used to record UMA stats.
  void RecordNumLoginAttempts(int num_attempt, bool success);
  void RecordUserTrayClick(TrayClickTarget target);
  void RecordUserShelfButtonClick(ShelfButtonClickTarget target);

 private:
  static AuthMethodSwitchType FindSwitchType(AuthMethod previous,
                                             AuthMethod current);

  AuthMethod last_auth_method_ = AuthMethod::kPassword;

  DISALLOW_COPY_AND_ASSIGN(LoginMetricsRecorder);
};

}  // namespace ash

#endif  // ASH_METRICS_LOGIN_METRICS_RECORDER_H_
