// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/service/display_embedder/compositing_mode_reporter_impl.h"

namespace viz {

CompositingModeReporterImpl::CompositingModeReporterImpl() = default;

CompositingModeReporterImpl::~CompositingModeReporterImpl() = default;

void CompositingModeReporterImpl::BindRequest(
    mojom::CompositingModeReporterRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void CompositingModeReporterImpl::SetUsingSoftwareCompositing() {
  gpu_ = false;
  watchers_.ForAllPtrs([](mojom::CompositingModeWatcher* watcher) {
    watcher->CompositingModeFallbackToSoftware();
  });
}

void CompositingModeReporterImpl::AddCompositingModeWatcher(
    mojom::CompositingModeWatcherPtr watcher) {
  if (!gpu_)
    watcher->CompositingModeFallbackToSoftware();
  watchers_.AddPtr(std::move(watcher));
}

}  // namespace viz
