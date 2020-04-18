// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SESSION_TELEPORT_WARNING_DIALOG_H_
#define ASH_SESSION_TELEPORT_WARNING_DIALOG_H_

#include "base/callback.h"
#include "ui/views/window/dialog_delegate.h"

namespace views {
class Checkbox;
}

namespace ash {

// Dialog for multi-profiles Window teleportation warning. When a user sends
// ("teleports") a window to another user's desktop, this dialog confirms that
// the user understands what that entails.
class TeleportWarningDialog : public views::DialogDelegateView {
 public:
  // This callback and its parameters match ShowTeleportWarningDialogCallback.
  typedef base::OnceCallback<void(bool, bool)> OnAcceptCallback;

  explicit TeleportWarningDialog(OnAcceptCallback callback);
  ~TeleportWarningDialog() override;

  static void Show(OnAcceptCallback callback);

  // views::DialogDelegate overrides.
  bool Cancel() override;
  bool Accept() override;

  // views::WidgetDelegate overrides.
  ui::ModalType GetModalType() const override;
  base::string16 GetWindowTitle() const override;
  bool ShouldShowCloseButton() const override;

  // views::View overrides.
  gfx::Size CalculatePreferredSize() const override;

 private:
  void InitDialog();

  views::Checkbox* never_show_again_checkbox_;

  OnAcceptCallback on_accept_;

  DISALLOW_COPY_AND_ASSIGN(TeleportWarningDialog);
};

}  // namespace ash

#endif  // ASH_SESSION_TELEPORT_WARNING_DIALOG_H_
