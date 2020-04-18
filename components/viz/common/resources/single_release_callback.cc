// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/common/resources/single_release_callback.h"

#include "base/callback_helpers.h"
#include "base/logging.h"

namespace viz {

SingleReleaseCallback::SingleReleaseCallback(ReleaseCallback callback)
    : callback_(std::move(callback)) {
  DCHECK(!callback_.is_null())
      << "Use a NULL SingleReleaseCallback for an empty callback.";
}

SingleReleaseCallback::~SingleReleaseCallback() {
  DCHECK(callback_.is_null()) << "SingleReleaseCallback was never run.";
}

void SingleReleaseCallback::Run(const gpu::SyncToken& sync_token,
                                bool is_lost) {
  DCHECK(!callback_.is_null())
      << "SingleReleaseCallback was run more than once.";
  std::move(callback_).Run(sync_token, is_lost);
}

}  // namespace viz
