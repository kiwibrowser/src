// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MEDIA_FLINGING_RENDERER_H_
#define CONTENT_BROWSER_MEDIA_FLINGING_RENDERER_H_

#include "base/callback.h"
#include "content/common/content_export.h"
#include "content/public/browser/media_controller.h"
#include "media/base/media_resource.h"
#include "media/base/renderer.h"
#include "media/base/renderer_client.h"
#include "url/gurl.h"

namespace content {

class FlingingRendererTest;
class RenderFrameHost;

// FlingingRenderer adapts from the media::Renderer interface to the
// MediaController interface. The MediaController is used to issue simple media
// playback commands. In this case, the media we are controlling should be an
// already existing RemotingCastSession, which should have been initiated by a
// blink::RemotePlayback object, using the PresentationService.
class CONTENT_EXPORT FlingingRenderer : public media::Renderer {
 public:
  // Helper method to create a FlingingRenderer from an already existing
  // presentation ID.
  // Returns nullptr if there was an error getting the MediaControllor for the
  // given presentation ID.
  static std::unique_ptr<FlingingRenderer> Create(
      RenderFrameHost* render_frame_host,
      const std::string& presentation_id);

  ~FlingingRenderer() override;

  // media::Renderer implementation
  void Initialize(media::MediaResource* media_resource,
                  media::RendererClient* client,
                  const media::PipelineStatusCB& init_cb) override;
  void SetCdm(media::CdmContext* cdm_context,
              const media::CdmAttachedCB& cdm_attached_cb) override;
  void Flush(const base::Closure& flush_cb) override;
  void StartPlayingFrom(base::TimeDelta time) override;
  void SetPlaybackRate(double playback_rate) override;
  void SetVolume(float volume) override;
  base::TimeDelta GetMediaTime() override;

 private:
  friend class FlingingRendererTest;

  explicit FlingingRenderer(std::unique_ptr<MediaController> controller);

  std::unique_ptr<MediaController> controller_;

  DISALLOW_COPY_AND_ASSIGN(FlingingRenderer);
};

}  // namespace content

#endif  // CONTENT_BROWSER_MEDIA_FLINGING_RENDERER_H_
