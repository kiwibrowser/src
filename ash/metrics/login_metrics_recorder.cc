// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/metrics/login_metrics_recorder.h"

#include "ash/login/ui/lock_screen.h"
#include "ash/session/session_controller.h"
#include "ash/shell.h"
#include "ash/wm/tablet_mode/tablet_mode_controller.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"

namespace ash {

namespace {

void LogUserClickOnLock(
    LoginMetricsRecorder::LockScreenUserClickTarget target) {
  DCHECK_NE(LoginMetricsRecorder::LockScreenUserClickTarget::kTargetCount,
            target);
  UMA_HISTOGRAM_ENUMERATION(
      "Ash.Login.Lock.UserClicks", target,
      LoginMetricsRecorder::LockScreenUserClickTarget::kTargetCount);
}

void LogUserClickOnLogin(
    LoginMetricsRecorder::LoginScreenUserClickTarget target) {
  DCHECK_NE(LoginMetricsRecorder::LoginScreenUserClickTarget::kTargetCount,
            target);
  UMA_HISTOGRAM_ENUMERATION(
      "Ash.Login.Login.UserClicks", target,
      LoginMetricsRecorder::LoginScreenUserClickTarget::kTargetCount);
}

void LogUserClick(
    LoginMetricsRecorder::LockScreenUserClickTarget lock_target,
    LoginMetricsRecorder::LoginScreenUserClickTarget login_target) {
  bool is_locked = Shell::Get()->session_controller()->GetSessionState() ==
                   session_manager::SessionState::LOCKED;
  if (is_locked) {
    LogUserClickOnLock(lock_target);
  } else {
    LogUserClickOnLogin(login_target);
  }
}

bool ShouldRecordMetrics() {
  session_manager::SessionState session_state =
      Shell::Get()->session_controller()->GetSessionState();
  return session_state == session_manager::SessionState::LOGIN_PRIMARY ||
         session_state == session_manager::SessionState::LOCKED;
}

}  // namespace

LoginMetricsRecorder::LoginMetricsRecorder() = default;
LoginMetricsRecorder::~LoginMetricsRecorder() = default;

void LoginMetricsRecorder::SetAuthMethod(AuthMethod method) {
  DCHECK_NE(method, AuthMethod::kMethodCount);
  if (Shell::Get()->session_controller()->GetSessionState() !=
      session_manager::SessionState::LOCKED)
    return;

  // Record usage of PIN / Password / Smartlock in lock screen.
  const bool is_tablet_mode = Shell::Get()
                                  ->tablet_mode_controller()
                                  ->IsTabletModeWindowManagerEnabled();
  if (is_tablet_mode) {
    UMA_HISTOGRAM_ENUMERATION("Ash.Login.Lock.AuthMethod.Used.TabletMode",
                              method, AuthMethod::kMethodCount);
  } else {
    UMA_HISTOGRAM_ENUMERATION("Ash.Login.Lock.AuthMethod.Used.ClamShellMode",
                              method, AuthMethod::kMethodCount);
  }

  if (last_auth_method_ != method) {
    // Record switching between unlock methods.
    UMA_HISTOGRAM_ENUMERATION("Ash.Login.Lock.AuthMethod.Switched",
                              FindSwitchType(last_auth_method_, method),
                              AuthMethodSwitchType::kSwitchTypeCount);

    last_auth_method_ = method;
  }
}

void LoginMetricsRecorder::Reset() {
  // Reset local state.
  last_auth_method_ = AuthMethod::kPassword;
}

void LoginMetricsRecorder::RecordNumLoginAttempts(int num_attempt,
                                                  bool success) {
  if (Shell::Get()->session_controller()->GetSessionState() !=
      session_manager::SessionState::LOCKED) {
    return;
  }

  if (success) {
    UMA_HISTOGRAM_COUNTS_100("Ash.Login.Lock.NumPasswordAttempts.UntilSuccess",
                             num_attempt);
  } else {
    UMA_HISTOGRAM_COUNTS_100("Ash.Login.Lock.NumPasswordAttempts.UntilFailure",
                             num_attempt);
  }
}

void LoginMetricsRecorder::RecordUserTrayClick(TrayClickTarget target) {
  if (!ShouldRecordMetrics())
    return;

  bool is_locked = Shell::Get()->session_controller()->GetSessionState() ==
                   session_manager::SessionState::LOCKED;
  switch (target) {
    case TrayClickTarget::kSystemTray:
      LogUserClick(LockScreenUserClickTarget::kSystemTray,
                   LoginScreenUserClickTarget::kSystemTray);
      break;
    case TrayClickTarget::kVirtualKeyboardTray:
      LogUserClick(LockScreenUserClickTarget::kVirtualKeyboardTray,
                   LoginScreenUserClickTarget::kVirtualKeyboardTray);
      break;
    case TrayClickTarget::kImeTray:
      LogUserClick(LockScreenUserClickTarget::kImeTray,
                   LoginScreenUserClickTarget::kImeTray);
      break;
    case TrayClickTarget::kNotificationTray:
      DCHECK(is_locked);
      LogUserClick(LockScreenUserClickTarget::kNotificationTray,
                   LoginScreenUserClickTarget::kTargetCount);
      break;
    case TrayClickTarget::kTrayActionNoteButton:
      DCHECK(is_locked);
      LogUserClick(LockScreenUserClickTarget::kLockScreenNoteActionButton,
                   LoginScreenUserClickTarget::kTargetCount);
      break;
    case TrayClickTarget::kTargetCount:
      NOTREACHED();
      break;
  }
}

void LoginMetricsRecorder::RecordUserShelfButtonClick(
    ShelfButtonClickTarget target) {
  if (!ShouldRecordMetrics())
    return;

  bool is_lock = Shell::Get()->session_controller()->GetSessionState() ==
                 session_manager::SessionState::LOCKED;
  switch (target) {
    case ShelfButtonClickTarget::kShutDownButton:
      LogUserClick(LockScreenUserClickTarget::kShutDownButton,
                   LoginScreenUserClickTarget::kShutDownButton);
      break;
    case ShelfButtonClickTarget::kRestartButton:
      LogUserClick(LockScreenUserClickTarget::kRestartButton,
                   LoginScreenUserClickTarget::kRestartButton);
      break;
    case ShelfButtonClickTarget::kSignOutButton:
      DCHECK(is_lock);
      LogUserClickOnLock(LockScreenUserClickTarget::kSignOutButton);
      break;
    case ShelfButtonClickTarget::kBrowseAsGuestButton:
      DCHECK(!is_lock);
      LogUserClickOnLogin(LoginScreenUserClickTarget::kBrowseAsGuestButton);
      break;
    case ShelfButtonClickTarget::kAddUserButton:
      DCHECK(!is_lock);
      LogUserClickOnLogin(LoginScreenUserClickTarget::kAddUserButton);
      break;
    case ShelfButtonClickTarget::kCloseNoteButton:
      DCHECK(is_lock);
      LogUserClickOnLock(LockScreenUserClickTarget::kCloseNoteButton);
      break;
    case ShelfButtonClickTarget::kCancelButton:
      // Should not be called in LOCKED nor LOGIN_PRIMARY states.
      NOTREACHED();
      break;
    case ShelfButtonClickTarget::kTargetCount:
      NOTREACHED();
      break;
  }
}

// static
LoginMetricsRecorder::AuthMethodSwitchType LoginMetricsRecorder::FindSwitchType(
    AuthMethod previous,
    AuthMethod current) {
  DCHECK_NE(previous, current);
  switch (previous) {
    case AuthMethod::kPassword:
      return current == AuthMethod::kPin
                 ? AuthMethodSwitchType::kPasswordToPin
                 : AuthMethodSwitchType::kPasswordToSmartlock;
    case AuthMethod::kPin:
      return current == AuthMethod::kPassword
                 ? AuthMethodSwitchType::kPinToPassword
                 : AuthMethodSwitchType::kPinToSmartlock;
    case AuthMethod::kSmartlock:
      return current == AuthMethod::kPassword
                 ? AuthMethodSwitchType::kSmartlockToPassword
                 : AuthMethodSwitchType::kSmartlockToPin;
    case AuthMethod::kMethodCount:
      NOTREACHED();
      return AuthMethodSwitchType::kSwitchTypeCount;
  }
}

}  // namespace ash
