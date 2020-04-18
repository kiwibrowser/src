// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/android/media_player_renderer_client_factory.h"

#include "content/renderer/media/android/media_player_renderer_client.h"
#include "media/mojo/clients/mojo_renderer.h"

namespace content {

MediaPlayerRendererClientFactory::MediaPlayerRendererClientFactory(
    scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner,
    std::unique_ptr<media::RendererFactory> mojo_renderer_factory,
    const GetStreamTextureWrapperCB& get_stream_texture_wrapper_cb)
    : get_stream_texture_wrapper_cb_(get_stream_texture_wrapper_cb),
      compositor_task_runner_(std::move(compositor_task_runner)),
      mojo_renderer_factory_(std::move(mojo_renderer_factory)) {}

MediaPlayerRendererClientFactory::~MediaPlayerRendererClientFactory() {}

std::unique_ptr<media::Renderer>
MediaPlayerRendererClientFactory::CreateRenderer(
    const scoped_refptr<base::SingleThreadTaskRunner>& media_task_runner,
    const scoped_refptr<base::TaskRunner>& worker_task_runner,
    media::AudioRendererSink* audio_renderer_sink,
    media::VideoRendererSink* video_renderer_sink,
    const media::RequestOverlayInfoCB& request_overlay_info_cb,
    const gfx::ColorSpace& target_color_space) {
  std::unique_ptr<media::Renderer> renderer =
      mojo_renderer_factory_->CreateRenderer(
          media_task_runner, worker_task_runner, audio_renderer_sink,
          video_renderer_sink, request_overlay_info_cb, target_color_space);

  media::MojoRenderer* mojo_renderer =
      static_cast<media::MojoRenderer*>(renderer.release());

  media::ScopedStreamTextureWrapper stream_texture_wrapper =
      get_stream_texture_wrapper_cb_.Run();

  return std::make_unique<MediaPlayerRendererClient>(
      media_task_runner, compositor_task_runner_, mojo_renderer,
      std::move(stream_texture_wrapper), video_renderer_sink);
}

media::MediaResource::Type
MediaPlayerRendererClientFactory::GetRequiredMediaResourceType() {
  return media::MediaResource::Type::URL;
}

}  // namespace content
