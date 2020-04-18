// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_ANDROID_MEDIA_PLAYER_RENDERER_CLIENT_H_
#define CONTENT_RENDERER_MEDIA_ANDROID_MEDIA_PLAYER_RENDERER_CLIENT_H_

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "content/common/content_export.h"
#include "media/base/android/stream_texture_wrapper.h"
#include "media/base/media_resource.h"
#include "media/base/renderer.h"
#include "media/base/renderer_client.h"
#include "media/base/video_renderer_sink.h"
#include "media/mojo/clients/mojo_renderer.h"

namespace content {

// MediaPlayerRendererClient lives in Renderer process and mirrors a
// MediaPlayerRenderer living in the Browser process.
//
// It is responsible for forwarding media::Renderer calls from WMPI to the
// MediaPlayerRenderer, using |mojo_renderer|. It also manages a StreamTexture,
// (via |stream_texture_wrapper_|) and notifies the VideoRendererSink when new
// frames are available.
//
// This class handles all calls on |media_task_runner_|, except for
// OnFrameAvailable(), which is called on |compositor_task_runner_|.
//
// N.B: This class implements media::RendererClient, in order to intercept
// OnVideoNaturalSizeChange() events, to update StreamTextureWrapper. All events
// (including OnVideoNaturalSizeChange()) are bubbled up to |client_|.
class CONTENT_EXPORT MediaPlayerRendererClient : public media::Renderer,
                                                 public media::RendererClient {
 public:
  MediaPlayerRendererClient(
      scoped_refptr<base::SingleThreadTaskRunner> media_task_runner,
      scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner,
      media::MojoRenderer* mojo_renderer,
      media::ScopedStreamTextureWrapper stream_texture_wrapper,
      media::VideoRendererSink* sink);

  ~MediaPlayerRendererClient() override;

  // media::Renderer implementation.
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

  // media::RendererClient implementation.
  void OnError(media::PipelineStatus status) override;
  void OnEnded() override;
  void OnStatisticsUpdate(const media::PipelineStatistics& stats) override;
  void OnBufferingStateChange(media::BufferingState state) override;
  void OnWaitingForDecryptionKey() override;
  void OnAudioConfigChange(const media::AudioDecoderConfig& config) override;
  void OnVideoConfigChange(const media::VideoDecoderConfig& config) override;
  void OnVideoNaturalSizeChange(const gfx::Size& size) override;
  void OnVideoOpacityChange(bool opaque) override;
  void OnDurationChange(base::TimeDelta duration) override;

  // Called on |compositor_task_runner_| whenever |stream_texture_wrapper_| has
  // a new frame.
  void OnFrameAvailable();

 private:
  void OnStreamTextureWrapperInitialized(media::MediaResource* media_resource,
                                         bool success);
  void OnRemoteRendererInitialized(media::PipelineStatus status);

  void OnScopedSurfaceRequested(const base::UnguessableToken& request_token);

  // Used to forward calls to the MediaPlayerRenderer living in the Browser.
  std::unique_ptr<media::MojoRenderer> mojo_renderer_;

  // Owns the StreamTexture whose surface is used by MediaPlayerRenderer.
  // Provides the VideoFrames to |sink_|.
  media::ScopedStreamTextureWrapper stream_texture_wrapper_;

  media::RendererClient* client_;

  media::VideoRendererSink* sink_;

  scoped_refptr<base::SingleThreadTaskRunner> media_task_runner_;

  // Used by |stream_texture_wrapper_| to signal OnFrameAvailable() and to send
  // VideoFrames to |sink_| on the right thread.
  scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner_;

  media::PipelineStatusCB init_cb_;

  // NOTE: Weak pointers must be invalidated before all other member variables.
  base::WeakPtrFactory<MediaPlayerRendererClient> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MediaPlayerRendererClient);
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_ANDROID_MEDIA_PLAYER_RENDERER_CLIENT_H_
