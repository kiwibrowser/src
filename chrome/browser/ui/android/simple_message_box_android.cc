// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/simple_message_box.h"

#include <utility>

#include "base/callback.h"
#include "base/logging.h"

namespace chrome {

void ShowWarningMessageBox(gfx::NativeWindow parent,
                           const base::string16& title,
                           const base::string16& message) {
  NOTIMPLEMENTED();
}

MessageBoxResult ShowQuestionMessageBox(gfx::NativeWindow parent,
                                        const base::string16& title,
                                        const base::string16& message) {
  NOTIMPLEMENTED();
  return MESSAGE_BOX_RESULT_NO;
}

void ShowWarningMessageBoxWithCheckbox(
    gfx::NativeWindow parent,
    const base::string16& title,
    const base::string16& message,
    const base::string16& checkbox_text,
    base::OnceCallback<void(bool checked)> callback) {
  NOTIMPLEMENTED();
  std::move(callback).Run(false);
}

}  // namespace chrome
