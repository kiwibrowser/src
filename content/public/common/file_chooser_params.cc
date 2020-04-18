// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/common/file_chooser_params.h"

namespace content {

FileChooserParams::FileChooserParams() : mode(Open), need_local_path(true) {
}

FileChooserParams::FileChooserParams(const FileChooserParams& other) = default;

FileChooserParams::~FileChooserParams() {
}

}  // namespace content
