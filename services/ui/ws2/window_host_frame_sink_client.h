// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS2_WINDOW_HOST_FRAME_SINK_CLIENT_H_
#define SERVICES_UI_WS2_WINDOW_HOST_FRAME_SINK_CLIENT_H_

#include "base/component_export.h"
#include "base/macros.h"
#include "components/viz/common/surfaces/local_surface_id.h"
#include "components/viz/common/surfaces/parent_local_surface_id_allocator.h"
#include "components/viz/host/host_frame_sink_client.h"

namespace aura {
class ClientSurfaceEmbedder;
}

namespace ui {
namespace ws2 {

// HostFrameSinkClient implementation used by WindowService. This is owned
// by WindowData.
class COMPONENT_EXPORT(WINDOW_SERVICE) WindowHostFrameSinkClient
    : public viz::HostFrameSinkClient {
 public:
  explicit WindowHostFrameSinkClient(
      aura::ClientSurfaceEmbedder* client_surface_embedder);
  ~WindowHostFrameSinkClient() override;

  // viz::HostFrameSinkClient:
  void OnFirstSurfaceActivation(const viz::SurfaceInfo& surface_info) override;
  void OnFrameTokenChanged(uint32_t frame_token) override;

 private:
  aura::ClientSurfaceEmbedder* client_surface_embedder_;

  DISALLOW_COPY_AND_ASSIGN(WindowHostFrameSinkClient);
};

}  // namespace ws2
}  // namespace ui

#endif  // SERVICES_UI_WS2_WINDOW_HOST_FRAME_SINK_CLIENT_H_
