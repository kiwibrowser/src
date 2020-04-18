// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/common/gpu/raster_context_provider.h"

namespace viz {

RasterContextProvider::ScopedRasterContextLock::ScopedRasterContextLock(
    RasterContextProvider* context_provider)
    : context_provider_(context_provider),
      context_lock_(*context_provider_->GetLock()) {
  busy_ = context_provider_->CacheController()->ClientBecameBusy();
}

RasterContextProvider::ScopedRasterContextLock::~ScopedRasterContextLock() {
  // Let ContextCacheController know we are no longer busy.
  context_provider_->CacheController()->ClientBecameNotBusy(std::move(busy_));
}

}  // namespace viz
