// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/null_resource_controller.h"

#include "base/logging.h"

namespace content {

NullResourceController::NullResourceController(bool* was_resumed)
    : was_resumed_(was_resumed) {
  *was_resumed = false;
}

NullResourceController::~NullResourceController() {}

void NullResourceController::Cancel() {
  DCHECK(!*was_resumed_);
}

void NullResourceController::CancelWithError(int error_code) {
  DCHECK(!*was_resumed_);
}

void NullResourceController::Resume() {
  *was_resumed_ = true;
}

}  // namespace content
