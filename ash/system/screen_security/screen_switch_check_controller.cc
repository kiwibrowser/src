// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/screen_security/screen_switch_check_controller.h"

#include "ash/shell.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/system/tray/system_tray_notifier.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/views/controls/message_box_view.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/window/dialog_delegate.h"

namespace ash {

namespace {

// Dialog that confirms the user wants to stop screen share/cast. Calls a
// callback with the result.
class CancelCastingDialog : public views::DialogDelegateView {
 public:
  CancelCastingDialog(base::OnceCallback<void(bool)> callback)
      : callback_(std::move(callback)) {
    AddChildView(new views::MessageBoxView(views::MessageBoxView::InitParams(
        l10n_util::GetStringUTF16(IDS_DESKTOP_CASTING_ACTIVE_MESSAGE))));
    SetLayoutManager(std::make_unique<views::FillLayout>());
  }
  ~CancelCastingDialog() override = default;

  base::string16 GetWindowTitle() const override {
    return l10n_util::GetStringUTF16(IDS_DESKTOP_CASTING_ACTIVE_TITLE);
  }

  int GetDialogButtons() const override {
    return ui::DIALOG_BUTTON_OK | ui::DIALOG_BUTTON_CANCEL;
  }

  bool Cancel() override {
    std::move(callback_).Run(false);
    return true;
  }

  bool Accept() override {
    // Stop screen sharing and capturing. When notified, all capture sessions or
    // all share sessions will be stopped.
    // Currently, the logic is in ScreenSecurityNotificationController.
    Shell::Get()->system_tray_notifier()->NotifyScreenCaptureStop();
    Shell::Get()->system_tray_notifier()->NotifyScreenShareStop();

    std::move(callback_).Run(true);
    return true;
  }

 private:
  base::OnceCallback<void(bool)> callback_;

  DISALLOW_COPY_AND_ASSIGN(CancelCastingDialog);
};

}  // namespace

ScreenSwitchCheckController::ScreenSwitchCheckController() {
  Shell::Get()->system_tray_notifier()->AddScreenCaptureObserver(this);
  Shell::Get()->system_tray_notifier()->AddScreenShareObserver(this);
}

ScreenSwitchCheckController::~ScreenSwitchCheckController() {
  Shell::Get()->system_tray_notifier()->RemoveScreenShareObserver(this);
  Shell::Get()->system_tray_notifier()->RemoveScreenCaptureObserver(this);
}

void ScreenSwitchCheckController::CanSwitchAwayFromActiveUser(
    base::OnceCallback<void(bool)> callback) {
  // If neither screen sharing nor capturing is going on we can immediately
  // switch users.
  if (!has_capture_ && !has_share_) {
    std::move(callback).Run(true);
    return;
  }

  views::DialogDelegate::CreateDialogWidget(
      new CancelCastingDialog(std::move(callback)),
      Shell::GetPrimaryRootWindow(), nullptr)
      ->Show();
}

void ScreenSwitchCheckController::OnScreenCaptureStart(
    const base::Closure& stop_callback,
    const base::string16& screen_capture_status) {
  has_capture_ = true;
}

void ScreenSwitchCheckController::OnScreenCaptureStop() {
  // Multiple screen capture sessions can exist, but they are stopped at once
  // for simplicity.
  has_capture_ = false;
}

void ScreenSwitchCheckController::OnScreenShareStart(
    const base::Closure& stop_callback,
    const base::string16& helper_name) {
  has_share_ = true;
}

void ScreenSwitchCheckController::OnScreenShareStop() {
  // Multiple screen share sessions can exist, but they are stopped at once for
  // simplicity.
  has_share_ = false;
}

}  // namespace ash
