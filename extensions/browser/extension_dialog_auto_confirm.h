// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_EXTENSION_DIALOG_AUTO_CONFIRM_H_
#define EXTENSIONS_BROWSER_EXTENSION_DIALOG_AUTO_CONFIRM_H_

#include "base/auto_reset.h"
#include "base/macros.h"

namespace extensions {

class ScopedTestDialogAutoConfirm {
 public:
  enum AutoConfirm {
    NONE,               // The prompt will show normally.
    ACCEPT,             // The prompt will always accept.
    ACCEPT_AND_OPTION,  // The prompt will always check an option (if any)
                        // and accept.
    CANCEL,             // The prompt will always cancel.
  };

  explicit ScopedTestDialogAutoConfirm(AutoConfirm override_value);
  ~ScopedTestDialogAutoConfirm();

  static AutoConfirm GetAutoConfirmValue();

 private:
  AutoConfirm old_value_;

  DISALLOW_COPY_AND_ASSIGN(ScopedTestDialogAutoConfirm);
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_EXTENSION_DIALOG_AUTO_CONFIRM_H_
