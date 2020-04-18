// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_LOCAL_LAYER_TREE_FRAME_SINK_LOCAL_H_
#define UI_AURA_LOCAL_LAYER_TREE_FRAME_SINK_LOCAL_H_

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "cc/trees/layer_tree_frame_sink.h"
#include "components/viz/common/frame_sinks/begin_frame_source.h"
#include "components/viz/common/surfaces/frame_sink_id.h"
#include "components/viz/host/host_frame_sink_client.h"
#include "services/viz/public/interfaces/compositing/compositor_frame_sink.mojom.h"
#include "ui/aura/window_port.h"
#include "ui/base/property_data.h"

namespace viz {
class CompositorFrameSinkSupport;
class HostFrameSinkManager;
}

namespace aura {

// cc::LayerTreeFrameSink implementation for classic aura, e.g. not mus.
// aura::Window::CreateLayerTreeFrameSink creates this class for a given
// aura::Window, and then the sink can be used for submitting frames to the
// aura::Window's ui::Layer.
class LayerTreeFrameSinkLocal : public cc::LayerTreeFrameSink,
                                public viz::mojom::CompositorFrameSinkClient,
                                public viz::ExternalBeginFrameSourceClient,
                                public viz::HostFrameSinkClient {
 public:
  LayerTreeFrameSinkLocal(const viz::FrameSinkId& frame_sink_id,
                          viz::HostFrameSinkManager* host_frame_sink_manager,
                          const std::string& debug_label);
  ~LayerTreeFrameSinkLocal() override;

  using SurfaceChangedCallback = base::Callback<void(const viz::SurfaceInfo&)>;

  // Set a callback which will be called when the surface is changed.
  void SetSurfaceChangedCallback(const SurfaceChangedCallback& callback);

  const viz::LocalSurfaceId& local_surface_id() const {
    return local_surface_id_;
  }

  // cc::LayerTreeFrameSink:
  bool BindToClient(cc::LayerTreeFrameSinkClient* client) override;
  void DetachFromClient() override;
  void SetLocalSurfaceId(const viz::LocalSurfaceId& local_surface_id) override;
  void SubmitCompositorFrame(viz::CompositorFrame frame) override;
  void DidNotProduceFrame(const viz::BeginFrameAck& ack) override;
  void DidAllocateSharedBitmap(mojo::ScopedSharedBufferHandle buffer,
                               const viz::SharedBitmapId& id) override;
  void DidDeleteSharedBitmap(const viz::SharedBitmapId& id) override;

  // viz::mojom::CompositorFrameSinkClient:
  void DidReceiveCompositorFrameAck(
      const std::vector<viz::ReturnedResource>& resources) override;
  void DidPresentCompositorFrame(uint32_t presentation_token,
                                 base::TimeTicks time,
                                 base::TimeDelta refresh,
                                 uint32_t flags) override;
  void DidDiscardCompositorFrame(uint32_t presentation_token) override;
  void OnBeginFrame(const viz::BeginFrameArgs& args) override;
  void ReclaimResources(
      const std::vector<viz::ReturnedResource>& resources) override;
  void OnBeginFramePausedChanged(bool paused) override;

  // viz::ExternalBeginFrameSourceClient:
  void OnNeedsBeginFrames(bool needs_begin_frames) override;

 private:
  // public viz::HostFrameSinkClient:
  void OnFirstSurfaceActivation(const viz::SurfaceInfo& surface_info) override;
  void OnFrameTokenChanged(uint32_t frame_token) override;

  const viz::FrameSinkId frame_sink_id_;
  viz::HostFrameSinkManager* const host_frame_sink_manager_;
  std::unique_ptr<viz::CompositorFrameSinkSupport> support_;
  gfx::Size surface_size_;
  viz::LocalSurfaceId local_surface_id_;
  std::unique_ptr<viz::ExternalBeginFrameSource> begin_frame_source_;
  std::unique_ptr<base::ThreadChecker> thread_checker_;
  SurfaceChangedCallback surface_changed_callback_;

  DISALLOW_COPY_AND_ASSIGN(LayerTreeFrameSinkLocal);
};

}  // namespace aura

#endif  // UI_AURA_LOCAL_LAYER_TREE_FRAME_SINK_LOCAL_H_
