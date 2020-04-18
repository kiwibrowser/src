// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_MODELS_DIALOG_MODEL_H_
#define UI_BASE_MODELS_DIALOG_MODEL_H_

#include "base/strings/string16.h"
#include "ui/base/ui_base_export.h"
#include "ui/base/ui_base_types.h"

namespace ui {

// A model representing a dialog window. The model provides the content to show
// to the user (i.e. title), and the ways the user can interact with it
// (i.e. the buttons).
class UI_BASE_EXPORT DialogModel {
 public:
  virtual ~DialogModel();

  // Returns a mask specifying which of the available DialogButtons are visible
  // for the dialog. Note: Dialogs with just an OK button are frowned upon.
  virtual int GetDialogButtons() const = 0;

  // Returns the default dialog button. This should not be a mask as only
  // one button should ever be the default button.  Return
  // ui::DIALOG_BUTTON_NONE if there is no default.  Default
  // behavior is to return ui::DIALOG_BUTTON_OK or
  // ui::DIALOG_BUTTON_CANCEL (in that order) if they are
  // present, ui::DIALOG_BUTTON_NONE otherwise.
  virtual int GetDefaultDialogButton() const = 0;

  // Returns the label of the specified dialog button.
  virtual base::string16 GetDialogButtonLabel(DialogButton button) const = 0;

  // Returns whether the specified dialog button is enabled.
  virtual bool IsDialogButtonEnabled(DialogButton button) const = 0;
};

}  // namespace ui

#endif  // UI_BASE_MODELS_DIALOG_MODEL_H_
