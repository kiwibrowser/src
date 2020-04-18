// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/common/frame_navigate_params.h"

namespace content {

FrameNavigateParams::FrameNavigateParams()
    : nav_entry_id(0),
      item_sequence_number(-1),
      document_sequence_number(-1),
      transition(ui::PAGE_TRANSITION_LINK),
      should_update_history(false) {
}

FrameNavigateParams::FrameNavigateParams(const FrameNavigateParams& other) =
    default;

FrameNavigateParams::~FrameNavigateParams() {
}

}  // namespace content
