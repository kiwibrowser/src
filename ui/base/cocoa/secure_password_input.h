// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_COCOA_SECURE_PASSWORD_INPUT_H_
#define UI_BASE_COCOA_SECURE_PASSWORD_INPUT_H_

#include "base/macros.h"
#include "ui/base/ui_base_export.h"

namespace ui {

// Enables the secure password input mode while in scope.
class UI_BASE_EXPORT ScopedPasswordInputEnabler {
 public:
  ScopedPasswordInputEnabler();
  ~ScopedPasswordInputEnabler();

  // Returns true if the password input mode is currently enabled. Useful for
  // unit tests.
  static bool IsPasswordInputEnabled();

 private:
  DISALLOW_COPY_AND_ASSIGN(ScopedPasswordInputEnabler);
};

}  // namespace ui

#endif  // UI_BASE_COCOA_SECURE_PASSWORD_INPUT_H_
