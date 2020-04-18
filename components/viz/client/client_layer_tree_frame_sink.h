// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_CLIENT_CLIENT_LAYER_TREE_FRAME_SINK_H_
#define COMPONENTS_VIZ_CLIENT_CLIENT_LAYER_TREE_FRAME_SINK_H_

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "cc/trees/layer_tree_frame_sink.h"
#include "components/viz/client/viz_client_export.h"
#include "components/viz/common/frame_sinks/begin_frame_source.h"
#include "components/viz/common/gpu/context_provider.h"
#include "components/viz/common/surfaces/parent_local_surface_id_allocator.h"
#include "components/viz/common/surfaces/surface_id.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/viz/public/interfaces/compositing/compositor_frame_sink.mojom.h"

namespace viz {

class HitTestDataProvider;
class LocalSurfaceIdProvider;

class VIZ_CLIENT_EXPORT ClientLayerTreeFrameSink
    : public cc::LayerTreeFrameSink,
      public mojom::CompositorFrameSinkClient,
      public ExternalBeginFrameSourceClient {
 public:
  struct VIZ_CLIENT_EXPORT UnboundMessagePipes {
    UnboundMessagePipes();
    ~UnboundMessagePipes();
    UnboundMessagePipes(UnboundMessagePipes&& other);

    bool HasUnbound() const;

    // Only one of |compositor_frame_sink_info| or
    // |compositor_frame_sink_associated_info| should be set.
    mojom::CompositorFrameSinkPtrInfo compositor_frame_sink_info;
    mojom::CompositorFrameSinkAssociatedPtrInfo
        compositor_frame_sink_associated_info;
    mojom::CompositorFrameSinkClientRequest client_request;
  };

  struct VIZ_CLIENT_EXPORT InitParams {
    InitParams();
    ~InitParams();

    scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner;
    gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager = nullptr;
    std::unique_ptr<SyntheticBeginFrameSource> synthetic_begin_frame_source;
    std::unique_ptr<HitTestDataProvider> hit_test_data_provider;
    std::unique_ptr<LocalSurfaceIdProvider> local_surface_id_provider;
    UnboundMessagePipes pipes;
    bool enable_surface_synchronization = false;
    bool wants_animate_only_begin_frames = false;
  };

  ClientLayerTreeFrameSink(
      scoped_refptr<ContextProvider> context_provider,
      scoped_refptr<RasterContextProvider> worker_context_provider,
      InitParams* params);

  ~ClientLayerTreeFrameSink() override;

  const HitTestDataProvider* hit_test_data_provider() const {
    return hit_test_data_provider_.get();
  }

  const LocalSurfaceId& local_surface_id() const { return local_surface_id_; }

  // cc::LayerTreeFrameSink implementation.
  bool BindToClient(cc::LayerTreeFrameSinkClient* client) override;
  void DetachFromClient() override;
  void SetLocalSurfaceId(const LocalSurfaceId& local_surface_id) override;
  void SubmitCompositorFrame(CompositorFrame frame) override;
  void DidNotProduceFrame(const BeginFrameAck& ack) override;
  void DidAllocateSharedBitmap(mojo::ScopedSharedBufferHandle buffer,
                               const SharedBitmapId& id) override;
  void DidDeleteSharedBitmap(const SharedBitmapId& id) override;

 private:
  // mojom::CompositorFrameSinkClient implementation:
  void DidReceiveCompositorFrameAck(
      const std::vector<ReturnedResource>& resources) override;
  void DidPresentCompositorFrame(uint32_t presentation_token,
                                 base::TimeTicks time,
                                 base::TimeDelta refresh,
                                 uint32_t flags) override;
  void DidDiscardCompositorFrame(uint32_t presentation_token) override;
  void OnBeginFrame(const BeginFrameArgs& begin_frame_args) override;
  void OnBeginFramePausedChanged(bool paused) override;
  void ReclaimResources(
      const std::vector<ReturnedResource>& resources) override;

  // ExternalBeginFrameSourceClient implementation.
  void OnNeedsBeginFrames(bool needs_begin_frames) override;

  void OnMojoConnectionError(uint32_t custom_reason,
                             const std::string& description);

  bool begin_frames_paused_ = false;
  bool needs_begin_frames_ = false;
  LocalSurfaceId local_surface_id_;
  std::unique_ptr<HitTestDataProvider> hit_test_data_provider_;
  std::unique_ptr<LocalSurfaceIdProvider> local_surface_id_provider_;
  std::unique_ptr<ExternalBeginFrameSource> begin_frame_source_;
  std::unique_ptr<SyntheticBeginFrameSource> synthetic_begin_frame_source_;

  // Message pipes that will be bound when BindToClient() is called.
  UnboundMessagePipes pipes_;

  // One of |compositor_frame_sink_| or |compositor_frame_sink_associated_| will
  // be bound after calling BindToClient(). |compositor_frame_sink_ptr_| will
  // point to message pipe we want to use.
  mojom::CompositorFrameSink* compositor_frame_sink_ptr_ = nullptr;
  mojom::CompositorFrameSinkPtr compositor_frame_sink_;
  mojom::CompositorFrameSinkAssociatedPtr compositor_frame_sink_associated_;
  mojo::Binding<mojom::CompositorFrameSinkClient> client_binding_;

  THREAD_CHECKER(thread_checker_);
  const bool enable_surface_synchronization_;
  const bool wants_animate_only_begin_frames_;

  LocalSurfaceId last_submitted_local_surface_id_;
  float last_submitted_device_scale_factor_ = 1.f;
  gfx::Size last_submitted_size_in_pixels_;

  base::WeakPtrFactory<ClientLayerTreeFrameSink> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ClientLayerTreeFrameSink);
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_CLIENT_CLIENT_LAYER_TREE_FRAME_SINK_H_
