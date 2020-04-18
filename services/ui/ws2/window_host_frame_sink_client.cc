// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws2/window_host_frame_sink_client.h"

#include "base/logging.h"
#include "ui/aura/mus/client_surface_embedder.h"

namespace ui {
namespace ws2 {

WindowHostFrameSinkClient::WindowHostFrameSinkClient(
    aura::ClientSurfaceEmbedder* client_surface_embedder)
    : client_surface_embedder_(client_surface_embedder) {}

WindowHostFrameSinkClient::~WindowHostFrameSinkClient() {}

void WindowHostFrameSinkClient::OnFirstSurfaceActivation(
    const viz::SurfaceInfo& surface_info) {
  client_surface_embedder_->SetFallbackSurfaceInfo(surface_info);
}

void WindowHostFrameSinkClient::OnFrameTokenChanged(uint32_t frame_token) {
  NOTIMPLEMENTED();
}

}  // namespace ws2
}  // namespace ui
