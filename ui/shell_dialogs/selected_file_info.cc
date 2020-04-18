// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/shell_dialogs/selected_file_info.h"

namespace ui {

SelectedFileInfo::SelectedFileInfo() {}

SelectedFileInfo::SelectedFileInfo(const base::FilePath& in_file_path,
                                   const base::FilePath& in_local_path)
    : file_path(in_file_path),
      local_path(in_local_path) {
  display_name = in_file_path.BaseName().value();
}

SelectedFileInfo::~SelectedFileInfo() {}

}  // namespace ui
