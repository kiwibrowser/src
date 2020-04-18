// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/test/mock_compositor_frame_sink_client.h"

#include "components/viz/common/frame_sinks/begin_frame_args.h"
#include "components/viz/common/surfaces/local_surface_id.h"
#include "ui/gfx/geometry/rect.h"

namespace viz {

MockCompositorFrameSinkClient::MockCompositorFrameSinkClient()
    : binding_(this) {}

MockCompositorFrameSinkClient::~MockCompositorFrameSinkClient() = default;

mojom::CompositorFrameSinkClientPtr
MockCompositorFrameSinkClient::BindInterfacePtr() {
  mojom::CompositorFrameSinkClientPtr ptr;
  binding_.Bind(MakeRequest(&ptr));
  return ptr;
}

}  // namespace viz
