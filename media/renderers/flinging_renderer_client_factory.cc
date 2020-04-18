// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/renderers/flinging_renderer_client_factory.h"

#include "base/logging.h"
#include "media/base/overlay_info.h"

namespace media {

FlingingRendererClientFactory::FlingingRendererClientFactory(
    std::unique_ptr<RendererFactory> mojo_flinging_factory,
    std::unique_ptr<RemotePlaybackClientWrapper> remote_playback_client)
    : mojo_flinging_factory_(std::move(mojo_flinging_factory)),
      remote_playback_client_(std::move(remote_playback_client)) {}

FlingingRendererClientFactory::~FlingingRendererClientFactory() = default;

std::unique_ptr<Renderer> FlingingRendererClientFactory::CreateRenderer(
    const scoped_refptr<base::SingleThreadTaskRunner>& media_task_runner,
    const scoped_refptr<base::TaskRunner>& worker_task_runner,
    AudioRendererSink* audio_renderer_sink,
    VideoRendererSink* video_renderer_sink,
    const RequestOverlayInfoCB& request_overlay_info_cb,
    const gfx::ColorSpace& target_color_space) {
  DCHECK(IsFlingingActive());
  return mojo_flinging_factory_->CreateRenderer(
      media_task_runner, worker_task_runner, audio_renderer_sink,
      video_renderer_sink, request_overlay_info_cb, target_color_space);
}

std::string FlingingRendererClientFactory::GetActivePresentationId() {
  return remote_playback_client_->GetActivePresentationId();
}

bool FlingingRendererClientFactory::IsFlingingActive() {
  return !GetActivePresentationId().empty();
}

}  // namespace media
