/// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/feature_engagement/public/tracker.h"

#include <utility>

namespace feature_engagement {

DisplayLockHandle::DisplayLockHandle(ReleaseCallback callback)
    : release_callback_(std::move(callback)) {}

DisplayLockHandle::~DisplayLockHandle() {
  if (release_callback_.is_null())
    return;

  std::move(release_callback_).Run();
}

}  // namespace feature_engagement
