// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/raster/tile_task.h"

#include "base/logging.h"

namespace cc {

TileTask::TileTask(bool supports_concurrent_execution)
    : supports_concurrent_execution_(supports_concurrent_execution),
      did_complete_(false) {}

TileTask::TileTask(bool supports_concurrent_execution,
                   TileTask::Vector* dependencies)
    : supports_concurrent_execution_(supports_concurrent_execution),
      dependencies_(std::move(*dependencies)),
      did_complete_(false) {}

TileTask::~TileTask() {
  DCHECK(did_complete_);
}

void TileTask::DidComplete() {
  DCHECK(!did_complete_);
  did_complete_ = true;
}

bool TileTask::HasCompleted() const {
  return did_complete_;
}

}  // namespace cc
