// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_SERVICE_FRAME_SINKS_ROOT_COMPOSITOR_FRAME_SINK_IMPL_H_
#define COMPONENTS_VIZ_SERVICE_FRAME_SINKS_ROOT_COMPOSITOR_FRAME_SINK_IMPL_H_

#include <memory>

#include "components/viz/common/surfaces/frame_sink_id.h"
#include "components/viz/common/surfaces/local_surface_id.h"
#include "components/viz/service/display/display_client.h"
#include "components/viz/service/frame_sinks/compositor_frame_sink_support.h"
#include "mojo/public/cpp/bindings/associated_binding.h"
#include "services/viz/privileged/interfaces/compositing/display_private.mojom.h"
#include "services/viz/public/interfaces/compositing/compositor_frame_sink.mojom.h"

namespace viz {

class Display;
class ExternalBeginFrameControllerImpl;
class FrameSinkManagerImpl;
class SyntheticBeginFrameSource;

// The viz portion of a root CompositorFrameSink. Holds the Binding/InterfacePtr
// for the mojom::CompositorFrameSink interface and owns the Display.
class RootCompositorFrameSinkImpl : public mojom::CompositorFrameSink,
                                    public mojom::DisplayPrivate,
                                    public DisplayClient {
 public:
  RootCompositorFrameSinkImpl(
      FrameSinkManagerImpl* frame_sink_manager,
      const FrameSinkId& frame_sink_id,
      std::unique_ptr<Display> display,
      std::unique_ptr<SyntheticBeginFrameSource> begin_frame_source,
      std::unique_ptr<ExternalBeginFrameControllerImpl>
          external_begin_frame_controller,
      mojom::CompositorFrameSinkAssociatedRequest request,
      mojom::CompositorFrameSinkClientPtr client,
      mojom::DisplayPrivateAssociatedRequest display_private_request,
      mojom::DisplayClientPtr display_client);

  ~RootCompositorFrameSinkImpl() override;

  // mojom::DisplayPrivate:
  void SetDisplayVisible(bool visible) override;
  void SetDisplayColorMatrix(const gfx::Transform& color_matrix) override;
  void SetDisplayColorSpace(const gfx::ColorSpace& blending_color_space,
                            const gfx::ColorSpace& device_color_space) override;
  void SetOutputIsSecure(bool secure) override;
  void SetAuthoritativeVSyncInterval(base::TimeDelta interval) override;
  void SetDisplayVSyncParameters(base::TimeTicks timebase,
                                 base::TimeDelta interval) override;

  // mojom::CompositorFrameSink:
  void SetNeedsBeginFrame(bool needs_begin_frame) override;
  void SetWantsAnimateOnlyBeginFrames() override;
  void SubmitCompositorFrame(
      const LocalSurfaceId& local_surface_id,
      CompositorFrame frame,
      base::Optional<HitTestRegionList> hit_test_region_list,
      uint64_t submit_time) override;
  void DidNotProduceFrame(const BeginFrameAck& begin_frame_ack) override;
  void DidAllocateSharedBitmap(mojo::ScopedSharedBufferHandle buffer,
                               const SharedBitmapId& id) override;
  void DidDeleteSharedBitmap(const SharedBitmapId& id) override;
  void SubmitCompositorFrameSync(
      const LocalSurfaceId& local_surface_id,
      CompositorFrame frame,
      base::Optional<HitTestRegionList> hit_test_region_list,
      uint64_t submit_time,
      SubmitCompositorFrameSyncCallback callback) override;

 private:
  // DisplayClient:
  void DisplayOutputSurfaceLost() override;
  void DisplayWillDrawAndSwap(bool will_draw_and_swap,
                              const RenderPassList& render_passes) override;
  void DisplayDidDrawAndSwap() override;
  void DisplayDidReceiveCALayerParams(
      const gfx::CALayerParams& ca_layer_params) override;
  void DidSwapAfterSnapshotRequestReceived(
      const std::vector<ui::LatencyInfo>& latency_info) override;

  void OnClientConnectionLost();

  BeginFrameSource* begin_frame_source();

  mojom::CompositorFrameSinkClientPtr compositor_frame_sink_client_;
  mojo::AssociatedBinding<mojom::CompositorFrameSink>
      compositor_frame_sink_binding_;
  // |display_client_| may be nullptr on platforms that do not use it.
  mojom::DisplayClientPtr display_client_;
  mojo::AssociatedBinding<mojom::DisplayPrivate> display_private_binding_;

  // Must be destroyed before |compositor_frame_sink_client_|. This must never
  // change for the lifetime of RootCompositorFrameSinkImpl.
  const std::unique_ptr<CompositorFrameSinkSupport> support_;

  // RootCompositorFrameSinkImpl holds a Display and its BeginFrameSource if
  // it was created with a non-null gpu::SurfaceHandle.
  std::unique_ptr<SyntheticBeginFrameSource> synthetic_begin_frame_source_;
  // If non-null, |synthetic_begin_frame_source_| will not exist.
  std::unique_ptr<ExternalBeginFrameControllerImpl>
      external_begin_frame_controller_;
  std::unique_ptr<Display> display_;

  DISALLOW_COPY_AND_ASSIGN(RootCompositorFrameSinkImpl);
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_SERVICE_FRAME_SINKS_ROOT_COMPOSITOR_FRAME_SINK_IMPL_H_
