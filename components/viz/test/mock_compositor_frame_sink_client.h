// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_TEST_MOCK_COMPOSITOR_FRAME_SINK_CLIENT_H_
#define COMPONENTS_VIZ_TEST_MOCK_COMPOSITOR_FRAME_SINK_CLIENT_H_

#include "base/callback.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/viz/public/interfaces/compositing/compositor_frame_sink.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace viz {

class MockCompositorFrameSinkClient : public mojom::CompositorFrameSinkClient {
 public:
  MockCompositorFrameSinkClient();
  ~MockCompositorFrameSinkClient() override;

  void set_connection_error_handler(base::OnceClosure error_handler) {
    binding_.set_connection_error_handler(std::move(error_handler));
  }

  // Returns a CompositorFrameSinkClientPtr bound to this object.
  mojom::CompositorFrameSinkClientPtr BindInterfacePtr();

  // mojom::CompositorFrameSinkClient implementation.
  MOCK_METHOD1(DidReceiveCompositorFrameAck,
               void(const std::vector<ReturnedResource>&));
  MOCK_METHOD4(DidPresentCompositorFrame,
               void(uint32_t, base::TimeTicks, base::TimeDelta, uint32_t));
  MOCK_METHOD1(DidDiscardCompositorFrame, void(uint32_t));
  MOCK_METHOD1(OnBeginFrame, void(const BeginFrameArgs&));
  MOCK_METHOD1(ReclaimResources, void(const std::vector<ReturnedResource>&));
  MOCK_METHOD2(WillDrawSurface, void(const LocalSurfaceId&, const gfx::Rect&));
  MOCK_METHOD1(OnBeginFramePausedChanged, void(bool paused));

 private:
  mojo::Binding<mojom::CompositorFrameSinkClient> binding_;

  DISALLOW_COPY_AND_ASSIGN(MockCompositorFrameSinkClient);
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_TEST_MOCK_COMPOSITOR_FRAME_SINK_CLIENT_H_
