// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_LAYER_TREE_FRAME_SINK_HOLDER_H_
#define COMPONENTS_EXO_LAYER_TREE_FRAME_SINK_HOLDER_H_

#include <memory>

#include "ash/shell_observer.h"
#include "base/containers/flat_map.h"
#include "cc/trees/layer_tree_frame_sink_client.h"
#include "components/viz/common/quads/compositor_frame.h"
#include "components/viz/common/resources/release_callback.h"

namespace ash {
class Shell;
}

namespace cc {
class LayerTreeFrameSink;
}

namespace exo {
class SurfaceTreeHost;

// This class talks to CompositorFrameSink and keeps track of references to
// the contents of Buffers.
class LayerTreeFrameSinkHolder : public cc::LayerTreeFrameSinkClient,
                                 public ash::ShellObserver {
 public:
  LayerTreeFrameSinkHolder(SurfaceTreeHost* surface_tree_host,
                           std::unique_ptr<cc::LayerTreeFrameSink> frame_sink);
  ~LayerTreeFrameSinkHolder() override;

  // Delete frame sink after having reclaimed and called all resource
  // release callbacks.
  // TODO(reveman): Find a better way to handle deletion of in-flight resources.
  // crbug.com/765763
  static void DeleteWhenLastResourceHasBeenReclaimed(
      std::unique_ptr<LayerTreeFrameSinkHolder> holder);

  void SubmitCompositorFrame(viz::CompositorFrame frame);
  void DidNotProduceFrame(const viz::BeginFrameAck& ack);

  bool HasReleaseCallbackForResource(viz::ResourceId id);
  void SetResourceReleaseCallback(viz::ResourceId id,
                                  viz::ReleaseCallback callback);
  int AllocateResourceId();
  base::WeakPtr<LayerTreeFrameSinkHolder> GetWeakPtr();

  // Overridden from cc::LayerTreeFrameSinkClient:
  void SetBeginFrameSource(viz::BeginFrameSource* source) override;
  base::Optional<viz::HitTestRegionList> BuildHitTestData() override;
  void ReclaimResources(
      const std::vector<viz::ReturnedResource>& resources) override;
  void SetTreeActivationCallback(const base::Closure& callback) override {}
  void DidReceiveCompositorFrameAck() override;
  void DidPresentCompositorFrame(uint32_t presentation_token,
                                 base::TimeTicks time,
                                 base::TimeDelta refresh,
                                 uint32_t flags) override;
  void DidDiscardCompositorFrame(uint32_t presentation_token) override;
  void DidLoseLayerTreeFrameSink() override;
  void OnDraw(const gfx::Transform& transform,
              const gfx::Rect& viewport,
              bool resourceless_software_draw) override {}
  void SetMemoryPolicy(const cc::ManagedMemoryPolicy& policy) override {}
  void SetExternalTilePriorityConstraints(
      const gfx::Rect& viewport_rect,
      const gfx::Transform& transform) override {}

  // Overridden from ash::ShellObserver:
  void OnShellDestroyed() override;

 private:
  void ScheduleDelete();

  // A collection of callbacks used to release resources.
  using ResourceReleaseCallbackMap =
      base::flat_map<viz::ResourceId, viz::ReleaseCallback>;
  ResourceReleaseCallbackMap release_callbacks_;

  SurfaceTreeHost* surface_tree_host_;
  std::unique_ptr<cc::LayerTreeFrameSink> frame_sink_;
  ash::Shell* shell_ = nullptr;

  // The next resource id the buffer is attached to.
  int next_resource_id_ = 1;

  gfx::Size last_frame_size_in_pixels_;
  float last_frame_device_scale_factor_ = 1.0f;
  std::vector<viz::ResourceId> last_frame_resources_;

  bool delete_pending_ = false;

  base::WeakPtrFactory<LayerTreeFrameSinkHolder> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(LayerTreeFrameSinkHolder);
};

}  // namespace exo

#endif  // COMPONENTS_EXO_LAYER_TREE_FRAME_SINK_HOLDER_H_
