// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/common/file_chooser_file_info.h"

namespace content {

FileChooserFileInfo::FileChooserFileInfo() : length(0), is_directory(false) {
}

FileChooserFileInfo::FileChooserFileInfo(const FileChooserFileInfo& other) =
    default;

FileChooserFileInfo::~FileChooserFileInfo() {
}

}  // namespace content
