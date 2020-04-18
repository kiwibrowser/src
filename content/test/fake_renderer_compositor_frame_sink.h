// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_TEST_FAKE_RENDERER_COMPOSITOR_FRAME_SINK_H_
#define CONTENT_TEST_FAKE_RENDERER_COMPOSITOR_FRAME_SINK_H_

#include "mojo/public/cpp/bindings/binding.h"
#include "services/viz/public/interfaces/compositing/compositor_frame_sink.mojom.h"

namespace content {

// This class is given to RenderWidgetHost/RenderWidgetHostView unit tests
// instead of RendererCompositorFrameSink.
class FakeRendererCompositorFrameSink
    : public viz::mojom::CompositorFrameSinkClient {
 public:
  FakeRendererCompositorFrameSink(
      viz::mojom::CompositorFrameSinkPtr sink,
      viz::mojom::CompositorFrameSinkClientRequest request);
  ~FakeRendererCompositorFrameSink() override;

  bool did_receive_ack() { return did_receive_ack_; }
  std::vector<viz::ReturnedResource>& last_reclaimed_resources() {
    return last_reclaimed_resources_;
  }

  // viz::mojom::CompositorFrameSinkClient implementation.
  void DidReceiveCompositorFrameAck(
      const std::vector<viz::ReturnedResource>& resources) override;
  void DidPresentCompositorFrame(uint32_t presentation_token,
                                 base::TimeTicks time,
                                 base::TimeDelta refresh,
                                 uint32_t flags) override {}
  void DidDiscardCompositorFrame(uint32_t presentation_token) override {}
  void OnBeginFrame(const viz::BeginFrameArgs& args) override {}
  void OnBeginFramePausedChanged(bool paused) override {}
  void ReclaimResources(
      const std::vector<viz::ReturnedResource>& resources) override;

  // Resets test data.
  void Reset();

  // Runs all queued messages.
  void Flush();

 private:
  mojo::Binding<viz::mojom::CompositorFrameSinkClient> binding_;
  viz::mojom::CompositorFrameSinkPtr sink_;
  bool did_receive_ack_ = false;
  std::vector<viz::ReturnedResource> last_reclaimed_resources_;

  DISALLOW_COPY_AND_ASSIGN(FakeRendererCompositorFrameSink);
};

}  // namespace content

#endif  // CONTENT_TEST_FAKE_RENDERER_COMPOSITOR_FRAME_SINK_H_
