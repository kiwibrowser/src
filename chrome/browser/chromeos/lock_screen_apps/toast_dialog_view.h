// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOCK_SCREEN_APPS_TOAST_DIALOG_VIEW_H_
#define CHROME_BROWSER_CHROMEOS_LOCK_SCREEN_APPS_TOAST_DIALOG_VIEW_H_

#include "base/callback.h"
#include "base/macros.h"
#include "base/strings/string16.h"
#include "ui/views/bubble/bubble_dialog_delegate.h"

namespace lock_screen_apps {

// The system modal bubble dialog shown to the user when a lock screen app is
// first launched from the lock screen. The dialog will block the app UI until
// the uesr closes it.
class ToastDialogView : public views::BubbleDialogDelegateView {
 public:
  ToastDialogView(const base::string16& app_name,
                  base::OnceClosure dismissed_callback);
  ~ToastDialogView() override;

  // Shows the toast dialog.
  void Show();

  // views::WidgetDelegate:
  ui::ModalType GetModalType() const override;
  base::string16 GetWindowTitle() const override;

  // views::BubbleDialogDelegate:
  bool Close() override;
  void AddedToWidget() override;
  int GetDialogButtons() const override;
  bool ShouldShowCloseButton() const override;

 private:
  // The name of the app for which the dialog is shown.
  const base::string16 app_name_;

  // Callback to be called when the user closes the dialog.
  base::OnceClosure dismissed_callback_;

  DISALLOW_COPY_AND_ASSIGN(ToastDialogView);
};

}  // namespace lock_screen_apps

#endif  // CHROME_BROWSER_CHROMEOS_LOCK_SCREEN_APPS_TOAST_DIALOG_VIEW_H_
