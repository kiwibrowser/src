// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_APP_MODAL_NATIVE_APP_MODAL_DIALOG_H_
#define COMPONENTS_APP_MODAL_NATIVE_APP_MODAL_DIALOG_H_

#include "ui/gfx/native_widget_types.h"

namespace app_modal {


class NativeAppModalDialog {
 public:
  virtual ~NativeAppModalDialog() {}

  // Returns the buttons to be shown. See ui::DialogButton for which buttons can
  // be returned.
  virtual int GetAppModalDialogButtons() const = 0;

  // Shows the dialog.
  virtual void ShowAppModalDialog() = 0;

  // Activates the dialog.
  virtual void ActivateAppModalDialog() = 0;

  // Closes the dialog.
  virtual void CloseAppModalDialog() = 0;

  // Accepts or cancels the dialog.
  virtual void AcceptAppModalDialog() = 0;
  virtual void CancelAppModalDialog() = 0;

  // Should return true when the dialog is being shown.
  virtual bool IsShowing() const = 0;
};

}  // namespace app_modal

#endif  // COMPONENTS_APP_MODAL_NATIVE_APP_MODAL_DIALOG_H_
