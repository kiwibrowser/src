// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_COMPOSITOR_FRAME_SINK_CLIENT_BINDING_H_
#define SERVICES_UI_WS_COMPOSITOR_FRAME_SINK_CLIENT_BINDING_H_

#include "base/macros.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/viz/privileged/interfaces/compositing/frame_sink_manager.mojom.h"
#include "services/viz/public/interfaces/compositing/compositor_frame_sink.mojom.h"

namespace ui {
namespace ws {

// CompositorFrameSinkClientBinding manages the binding between a FrameGenerator
// and its CompositorFrameSink. CompositorFrameSinkClientBinding exists so
// that a mock implementation of CompositorFrameSink can be injected for
// tests. FrameGenerator owns its associated CompositorFrameSinkClientBinding.
class CompositorFrameSinkClientBinding
    : public viz::mojom::CompositorFrameSink {
 public:
  CompositorFrameSinkClientBinding(
      viz::mojom::CompositorFrameSinkClient* sink_client,
      viz::mojom::CompositorFrameSinkClientRequest sink_client_request,
      viz::mojom::CompositorFrameSinkAssociatedPtr compositor_frame_sink,
      viz::mojom::DisplayPrivateAssociatedPtr display_private);
  ~CompositorFrameSinkClientBinding() override;

 private:
  // viz::mojom::CompositorFrameSink implementation:
  void SubmitCompositorFrame(
      const viz::LocalSurfaceId& local_surface_id,
      viz::CompositorFrame frame,
      base::Optional<viz::HitTestRegionList> hit_test_region_list,
      uint64_t submit_time) override;
  void SubmitCompositorFrameSync(
      const viz::LocalSurfaceId& local_surface_id,
      viz::CompositorFrame frame,
      base::Optional<viz::HitTestRegionList> hit_test_region_list,
      uint64_t submit_time,
      const SubmitCompositorFrameSyncCallback callback) override;
  void SetNeedsBeginFrame(bool needs_begin_frame) override;
  void SetWantsAnimateOnlyBeginFrames() override;
  void DidNotProduceFrame(const viz::BeginFrameAck& ack) override;
  void DidAllocateSharedBitmap(mojo::ScopedSharedBufferHandle buffer,
                               const viz::SharedBitmapId& id) override;
  void DidDeleteSharedBitmap(const viz::SharedBitmapId& id) override;

  mojo::Binding<viz::mojom::CompositorFrameSinkClient> binding_;
  viz::mojom::DisplayPrivateAssociatedPtr display_private_;
  viz::mojom::CompositorFrameSinkAssociatedPtr compositor_frame_sink_;

  DISALLOW_COPY_AND_ASSIGN(CompositorFrameSinkClientBinding);
};
}
}

#endif  // SERVICES_UI_WS_COMPOSITOR_FRAME_SINK_CLIENT_BINDING_H_
