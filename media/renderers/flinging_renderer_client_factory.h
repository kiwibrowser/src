// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_RENDERERS_FLINGING_RENDERER_CLIENT_FACTORY_H_
#define MEDIA_RENDERERS_FLINGING_RENDERER_CLIENT_FACTORY_H_

#include "media/base/media_export.h"
#include "media/base/renderer_factory.h"
#include "media/renderers/remote_playback_client_wrapper.h"

namespace media {

// Creates a renderer for media flinging.
// The FRCF uses a MojoRendererFactory to create a FlingingRenderer in the
// browser process. The actual renderer returned by the FRCF is a MojoRenderer
// directly (as opposed to a dedicated FlingingRendererClient), because all the
// renderer needs to do is forward calls to the FlingingRenderer in the browser.
class MEDIA_EXPORT FlingingRendererClientFactory : public RendererFactory {
 public:
  // |mojo_flinging_factory| should be created using
  // HostedRendererType::kFlinging, and GetActivePresentationId()
  // should be given to it through SetGetTypeSpecificIdCB().
  FlingingRendererClientFactory(
      std::unique_ptr<RendererFactory> mojo_flinging_factory,
      std::unique_ptr<RemotePlaybackClientWrapper> remote_playback_client);
  ~FlingingRendererClientFactory() override;

  std::unique_ptr<Renderer> CreateRenderer(
      const scoped_refptr<base::SingleThreadTaskRunner>& media_task_runner,
      const scoped_refptr<base::TaskRunner>& worker_task_runner,
      AudioRendererSink* audio_renderer_sink,
      VideoRendererSink* video_renderer_sink,
      const RequestOverlayInfoCB& request_overlay_info_cb,
      const gfx::ColorSpace& target_color_space) override;

  // Returns whether media flinging has started, based off of whether the
  // |remote_playback_client_| has a presentation ID or not. Called by
  // RendererFactorySelector to determine when to create a FlingingRenderer.
  bool IsFlingingActive();

  // Used by the |mojo_flinging_factory_| to retrieve the latest presentation ID
  // when CreateRenderer() is called.
  std::string GetActivePresentationId();

 private:
  std::unique_ptr<RendererFactory> mojo_flinging_factory_;
  std::unique_ptr<RemotePlaybackClientWrapper> remote_playback_client_;

  DISALLOW_COPY_AND_ASSIGN(FlingingRendererClientFactory);
};

}  // namespace media

#endif  // MEDIA_RENDERERS_FLINGING_RENDERER_CLIENT_FACTORY_H_
