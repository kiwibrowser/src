// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/test/mock_background_sync_controller.h"

namespace content {

void MockBackgroundSyncController::NotifyBackgroundSyncRegistered(
    const GURL& origin) {
  registration_count_ += 1;
  registration_origin_ = origin;
}

void MockBackgroundSyncController::RunInBackground(bool enabled,
                                                   int64_t min_ms) {
  run_in_background_count_ += 1;
  run_in_background_enabled_ = enabled;
  run_in_background_min_ms_ = min_ms;
}

void MockBackgroundSyncController::GetParameterOverrides(
    BackgroundSyncParameters* parameters) const {
  *parameters = background_sync_parameters_;
}

}  // namespace content
