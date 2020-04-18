// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/video_detector_impl.h"

#include "components/viz/host/host_frame_sink_manager.h"

namespace ui {
namespace ws {

VideoDetectorImpl::VideoDetectorImpl(
    viz::HostFrameSinkManager* host_frame_sink_manager)
    : host_frame_sink_manager_(host_frame_sink_manager) {}

VideoDetectorImpl::~VideoDetectorImpl() = default;

void VideoDetectorImpl::AddBinding(mojom::VideoDetectorRequest request) {
  binding_set_.AddBinding(this, std::move(request));
}

void VideoDetectorImpl::AddObserver(
    viz::mojom::VideoDetectorObserverPtr observer) {
  host_frame_sink_manager_->AddVideoDetectorObserver(std::move(observer));
}

}  // namespace ws
}  // namespace ui
