// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_SERVICE_DISPLAY_EMBEDDER_DISPLAY_PROVIDER_H_
#define COMPONENTS_VIZ_SERVICE_DISPLAY_EMBEDDER_DISPLAY_PROVIDER_H_

#include <memory>

#include "gpu/ipc/common/surface_handle.h"
#include "services/viz/privileged/interfaces/compositing/display_private.mojom.h"

namespace viz {

class Display;
class ExternalBeginFrameControllerImpl;
class FrameSinkId;
class RendererSettings;
class SyntheticBeginFrameSource;

// Handles creating Display and related classes for FrameSinkManagerImpl.
class DisplayProvider {
 public:
  virtual ~DisplayProvider() {}

  // Creates a new Display for |surface_handle| with |frame_sink_id|. Will
  // also create BeginFrameSource and return it in |begin_frame_source|.
  // Will return null if creating a Display failed.
  virtual std::unique_ptr<Display> CreateDisplay(
      const FrameSinkId& frame_sink_id,
      gpu::SurfaceHandle surface_handle,
      bool gpu_compositing,
      mojom::DisplayClient* display_client,
      ExternalBeginFrameControllerImpl* external_begin_frame_controller,
      const RendererSettings& renderer_settings,
      std::unique_ptr<SyntheticBeginFrameSource>* out_begin_frame_source) = 0;
};

}  // namespace viz

#endif  //  COMPONENTS_VIZ_SERVICE_DISPLAY_EMBEDDER_DISPLAY_PROVIDER_H_
