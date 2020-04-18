// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_TEST_TEST_DISPLAY_PROVIDER_H_
#define COMPONENTS_VIZ_TEST_TEST_DISPLAY_PROVIDER_H_

#include <memory>

#include "components/viz/service/display/display.h"
#include "components/viz/service/display_embedder/display_provider.h"
#include "components/viz/test/test_shared_bitmap_manager.h"

namespace viz {

// Test implementation that creates a Display with a FakeOutputSurface.
class TestDisplayProvider : public DisplayProvider {
 public:
  TestDisplayProvider();
  ~TestDisplayProvider() override;

  // DisplayProvider implementation.
  std::unique_ptr<Display> CreateDisplay(
      const FrameSinkId& frame_sink_id,
      gpu::SurfaceHandle surface_handle,
      bool gpu_compositing,
      mojom::DisplayClient* display_client,
      ExternalBeginFrameControllerImpl* external_begin_frame_controller,
      const RendererSettings& renderer_settings,
      std::unique_ptr<SyntheticBeginFrameSource>* out_begin_frame_source)
      override;

 private:
  TestSharedBitmapManager shared_bitmap_manager_;

  DISALLOW_COPY_AND_ASSIGN(TestDisplayProvider);
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_TEST_TEST_DISPLAY_PROVIDER_H_
