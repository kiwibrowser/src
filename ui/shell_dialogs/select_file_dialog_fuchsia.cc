// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/shell_dialogs/select_file_dialog.h"

#include "base/logging.h"

namespace ui {

SelectFileDialog* CreateSelectFileDialog(
    SelectFileDialog::Listener* listener,
    std::unique_ptr<SelectFilePolicy> policy) {
  // TODO(fuchsia): Port once we have UI, see https://crbug.com/746674.
  NOTREACHED();
  return nullptr;
}

}  // namespace ui
