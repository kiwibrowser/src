// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/embedded_frame_sink_provider_impl.h"

#include "base/bind.h"
#include "components/viz/host/host_frame_sink_manager.h"
#include "content/browser/renderer_host/embedded_frame_sink_impl.h"

namespace content {

EmbeddedFrameSinkProviderImpl::EmbeddedFrameSinkProviderImpl(
    viz::HostFrameSinkManager* host_frame_sink_manager,
    uint32_t renderer_client_id)
    : host_frame_sink_manager_(host_frame_sink_manager),
      renderer_client_id_(renderer_client_id) {
  DCHECK(host_frame_sink_manager);
}

EmbeddedFrameSinkProviderImpl::~EmbeddedFrameSinkProviderImpl() = default;

void EmbeddedFrameSinkProviderImpl::Add(
    blink::mojom::EmbeddedFrameSinkProviderRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void EmbeddedFrameSinkProviderImpl::RegisterEmbeddedFrameSink(
    const viz::FrameSinkId& parent_frame_sink_id,
    const viz::FrameSinkId& frame_sink_id,
    blink::mojom::EmbeddedFrameSinkClientPtr client) {
  // TODO(kylechar): Kill the renderer too.
  if (parent_frame_sink_id.client_id() != renderer_client_id_) {
    DLOG(ERROR) << "Invalid parent client id " << parent_frame_sink_id;
    return;
  }
  if (frame_sink_id.client_id() != renderer_client_id_) {
    DLOG(ERROR) << "Invalid client id " << frame_sink_id;
    return;
  }

  auto destroy_callback =
      base::BindOnce(&EmbeddedFrameSinkProviderImpl::DestroyEmbeddedFrameSink,
                     base::Unretained(this), frame_sink_id);

  frame_sink_map_[frame_sink_id] = std::make_unique<EmbeddedFrameSinkImpl>(
      host_frame_sink_manager_, parent_frame_sink_id, frame_sink_id,
      std::move(client), std::move(destroy_callback));
}

void EmbeddedFrameSinkProviderImpl::CreateCompositorFrameSink(
    const viz::FrameSinkId& frame_sink_id,
    viz::mojom::CompositorFrameSinkClientPtr client,
    viz::mojom::CompositorFrameSinkRequest request) {
  // TODO(kylechar): Kill the renderer too.
  if (frame_sink_id.client_id() != renderer_client_id_) {
    DLOG(ERROR) << "Invalid client id " << frame_sink_id;
    return;
  }

  auto iter = frame_sink_map_.find(frame_sink_id);
  if (iter == frame_sink_map_.end()) {
    DLOG(ERROR) << "No EmbeddedFrameSinkImpl for " << frame_sink_id;
    return;
  }

  iter->second->CreateCompositorFrameSink(std::move(client),
                                          std::move(request));
}

void EmbeddedFrameSinkProviderImpl::CreateSimpleCompositorFrameSink(
    const viz::FrameSinkId& parent_frame_sink_id,
    const viz::FrameSinkId& frame_sink_id,
    blink::mojom::EmbeddedFrameSinkClientPtr embedded_frame_sink_client,
    viz::mojom::CompositorFrameSinkClientPtr compositor_frame_sink_client,
    viz::mojom::CompositorFrameSinkRequest compositor_frame_sink_request) {
  RegisterEmbeddedFrameSink(parent_frame_sink_id, frame_sink_id,
                            std::move(embedded_frame_sink_client));
  CreateCompositorFrameSink(frame_sink_id,
                            std::move(compositor_frame_sink_client),
                            std::move(compositor_frame_sink_request));
}

void EmbeddedFrameSinkProviderImpl::DestroyEmbeddedFrameSink(
    viz::FrameSinkId frame_sink_id) {
  frame_sink_map_.erase(frame_sink_id);
}

}  // namespace content
