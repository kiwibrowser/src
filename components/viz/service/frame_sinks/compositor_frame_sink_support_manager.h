// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_SERVICE_FRAME_SINKS_COMPOSITOR_FRAME_SINK_SUPPORT_MANAGER_H_
#define COMPONENTS_VIZ_SERVICE_FRAME_SINKS_COMPOSITOR_FRAME_SINK_SUPPORT_MANAGER_H_

#include <memory>

#include "base/callback.h"

namespace viz {

class CompositorFrameSinkSupport;
class FrameSinkId;

namespace mojom {
class CompositorFrameSinkClient;
}

// This inteface provides a way for DirectLayerTreeFrameSink to create a
// CompositorFrameSinkSupport.
class CompositorFrameSinkSupportManager {
 public:
  virtual std::unique_ptr<CompositorFrameSinkSupport>
  CreateCompositorFrameSinkSupport(mojom::CompositorFrameSinkClient* client,
                                   const FrameSinkId& frame_sink_id,
                                   bool is_root,
                                   bool needs_sync_points) = 0;

 protected:
  virtual ~CompositorFrameSinkSupportManager() {}
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_SERVICE_FRAME_SINKS_COMPOSITOR_FRAME_SINK_SUPPORT_MANAGER_H_
