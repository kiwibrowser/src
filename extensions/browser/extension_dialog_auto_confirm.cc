// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/extension_dialog_auto_confirm.h"

namespace extensions {

namespace {
ScopedTestDialogAutoConfirm::AutoConfirm g_extension_dialog_auto_confirm =
    ScopedTestDialogAutoConfirm::NONE;
}

ScopedTestDialogAutoConfirm::ScopedTestDialogAutoConfirm(
    ScopedTestDialogAutoConfirm::AutoConfirm override_value)
    : old_value_(g_extension_dialog_auto_confirm) {
  g_extension_dialog_auto_confirm = override_value;
}

ScopedTestDialogAutoConfirm::~ScopedTestDialogAutoConfirm() {
  g_extension_dialog_auto_confirm = old_value_;
}

ScopedTestDialogAutoConfirm::AutoConfirm
ScopedTestDialogAutoConfirm::GetAutoConfirmValue() {
  return g_extension_dialog_auto_confirm;
}

}  // namespace extensions
