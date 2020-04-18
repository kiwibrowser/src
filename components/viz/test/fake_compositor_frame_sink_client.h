// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_TEST_FAKE_COMPOSITOR_FRAME_SINK_CLIENT_H_
#define COMPONENTS_VIZ_TEST_FAKE_COMPOSITOR_FRAME_SINK_CLIENT_H_

#include <vector>

#include "components/viz/common/surfaces/local_surface_id.h"
#include "services/viz/public/interfaces/compositing/compositor_frame_sink.mojom.h"

namespace viz {

class FakeCompositorFrameSinkClient : public mojom::CompositorFrameSinkClient {
 public:
  FakeCompositorFrameSinkClient();
  ~FakeCompositorFrameSinkClient() override;

  // mojom::CompositorFrameSinkClient implementation.
  void DidReceiveCompositorFrameAck(
      const std::vector<ReturnedResource>& resources) override;
  void DidPresentCompositorFrame(uint32_t presentation_token,
                                 base::TimeTicks time,
                                 base::TimeDelta refresh,
                                 uint32_t flags) override;
  void DidDiscardCompositorFrame(uint32_t presentation_token) override;
  void OnBeginFrame(const BeginFrameArgs& args) override;
  void ReclaimResources(
      const std::vector<ReturnedResource>& resources) override;
  void OnBeginFramePausedChanged(bool paused) override;

  void clear_returned_resources() { returned_resources_.clear(); }
  const std::vector<ReturnedResource>& returned_resources() const {
    return returned_resources_;
  }

 private:
  void InsertResources(const std::vector<ReturnedResource>& resources);

  std::vector<ReturnedResource> returned_resources_;

  DISALLOW_COPY_AND_ASSIGN(FakeCompositorFrameSinkClient);
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_TEST_FAKE_COMPOSITOR_FRAME_SINK_CLIENT_H_
