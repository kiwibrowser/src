// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/test/test_frame_sink_manager_client.h"

namespace viz {

TestFrameSinkManagerClient::TestFrameSinkManagerClient(
    mojom::FrameSinkManager* manager)
    : manager_(manager) {}

TestFrameSinkManagerClient::~TestFrameSinkManagerClient() = default;

void TestFrameSinkManagerClient::SetFrameSinkHierarchy(
    const FrameSinkId& parent_frame_sink_id,
    const FrameSinkId& child_frame_sink_id) {
  frame_sink_hierarchy_[child_frame_sink_id] = parent_frame_sink_id;
}

void TestFrameSinkManagerClient::DisableAssignTemporaryReferences() {
  disabled_ = true;
}

void TestFrameSinkManagerClient::Reset() {
  disabled_ = false;
  frame_sink_hierarchy_.clear();
}

void TestFrameSinkManagerClient::OnSurfaceCreated(const SurfaceId& surface_id) {
  if (disabled_)
    return;

  auto iter = frame_sink_hierarchy_.find(surface_id.frame_sink_id());
  if (iter == frame_sink_hierarchy_.end()) {
    manager_->DropTemporaryReference(surface_id);
    return;
  }

  manager_->AssignTemporaryReference(surface_id, iter->second);
}

}  // namespace viz
