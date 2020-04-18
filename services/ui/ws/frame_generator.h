// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_FRAME_GENERATOR_H_
#define SERVICES_UI_WS_FRAME_GENERATOR_H_

#include <memory>

#include "base/macros.h"
#include "components/viz/common/frame_sinks/begin_frame_source.h"
#include "components/viz/common/hit_test/hit_test_region_list.h"
#include "components/viz/common/surfaces/parent_local_surface_id_allocator.h"
#include "components/viz/common/surfaces/surface_id.h"
#include "components/viz/common/surfaces/surface_info.h"
#include "services/ui/ws/compositor_frame_sink_client_binding.h"
#include "services/viz/public/interfaces/compositing/compositor_frame_sink.mojom.h"
#include "ui/gfx/geometry/rect.h"

namespace cc {
class RenderPass;
}

namespace ui {
namespace ws {

// Responsible for redrawing the display in response to the redraw requests by
// submitting CompositorFrames to the owned CompositorFrameSink.
class FrameGenerator : public viz::mojom::CompositorFrameSinkClient {
 public:
  FrameGenerator();
  ~FrameGenerator() override;

  void SetDeviceScaleFactor(float device_scale_factor);
  void SetHighContrastMode(bool enabled);

  // Updates the WindowManager's SurfaceInfo.
  void SetEmbeddedSurface(const viz::SurfaceInfo& surface_info);

  // Swaps the |window_manager_surface_info_| with that of |other|.
  void SwapSurfaceWith(FrameGenerator* other);

  void OnWindowDamaged();
  void OnWindowSizeChanged(const gfx::Size& pixel_size);
  void Bind(
      std::unique_ptr<viz::mojom::CompositorFrameSink> compositor_frame_sink);

  const viz::SurfaceInfo& window_manager_surface_info() const {
    return window_manager_surface_info_;
  }

  void set_scale_and_center(bool value) { scale_and_center_ = value; }

 private:
  // viz::mojom::CompositorFrameSinkClient implementation:
  void DidReceiveCompositorFrameAck(
      const std::vector<viz::ReturnedResource>& resources) override;
  void DidPresentCompositorFrame(uint32_t presentation_token,
                                 base::TimeTicks time,
                                 base::TimeDelta refresh,
                                 uint32_t flags) override;
  void DidDiscardCompositorFrame(uint32_t presentation_token) override;
  void OnBeginFrame(const viz::BeginFrameArgs& args) override;
  void OnBeginFramePausedChanged(bool paused) override {}
  void ReclaimResources(
      const std::vector<viz::ReturnedResource>& resources) override;

  viz::CompositorFrame GenerateCompositorFrame();

  viz::HitTestRegionList GenerateHitTestRegionList() const;

  // DrawWindow creates SurfaceDrawQuad for the window manager and appends it to
  // the provided viz::RenderPass.
  void DrawWindow(viz::RenderPass* pass);

  void SetNeedsBeginFrame(bool needs_begin_frame);

  float device_scale_factor_ = 1.f;
  gfx::Size pixel_size_;

  std::unique_ptr<viz::mojom::CompositorFrameSink> compositor_frame_sink_;
  viz::BeginFrameArgs last_begin_frame_args_;
  viz::BeginFrameAck current_begin_frame_ack_;
  bool high_contrast_mode_enabled_ = false;
  gfx::Size last_submitted_frame_size_;
  viz::ParentLocalSurfaceIdAllocator id_allocator_;
  float last_device_scale_factor_ = 0.0f;

  viz::SurfaceInfo window_manager_surface_info_;

  // Whether the window manager surface should be scaled and centered.
  bool scale_and_center_ = false;

  DISALLOW_COPY_AND_ASSIGN(FrameGenerator);
};

}  // namespace ws
}  // namespace ui

#endif  // SERVICES_UI_WS_FRAME_GENERATOR_H_
