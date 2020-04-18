// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/test/fake_compositor_frame_sink_client.h"

namespace viz {

FakeCompositorFrameSinkClient::FakeCompositorFrameSinkClient() = default;
FakeCompositorFrameSinkClient::~FakeCompositorFrameSinkClient() = default;

void FakeCompositorFrameSinkClient::DidReceiveCompositorFrameAck(
    const std::vector<ReturnedResource>& resources) {
  InsertResources(resources);
}

void FakeCompositorFrameSinkClient::DidPresentCompositorFrame(
    uint32_t presentation_token,
    base::TimeTicks time,
    base::TimeDelta refresh,
    uint32_t flags) {}

void FakeCompositorFrameSinkClient::DidDiscardCompositorFrame(
    uint32_t presentation_token) {}

void FakeCompositorFrameSinkClient::OnBeginFrame(const BeginFrameArgs& args) {}

void FakeCompositorFrameSinkClient::ReclaimResources(
    const std::vector<ReturnedResource>& resources) {
  InsertResources(resources);
}

void FakeCompositorFrameSinkClient::OnBeginFramePausedChanged(bool paused) {}

void FakeCompositorFrameSinkClient::InsertResources(
    const std::vector<ReturnedResource>& resources) {
  returned_resources_.insert(returned_resources_.end(), resources.begin(),
                             resources.end());
}

};  // namespace viz
