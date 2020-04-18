// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MEDIA_ANDROID_MEDIA_PLAYER_RENDERER_H_
#define CONTENT_BROWSER_MEDIA_ANDROID_MEDIA_PLAYER_RENDERER_H_

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/unguessable_token.h"
#include "content/common/content_export.h"
#include "content/public/browser/render_frame_host.h"
#include "media/base/android/media_player_bridge.h"
#include "media/base/android/media_player_manager.h"
#include "media/base/media_log.h"
#include "media/base/media_resource.h"
#include "media/base/renderer.h"
#include "media/base/renderer_client.h"
#include "url/gurl.h"

namespace content {

class WebContents;
class MediaPlayerRendererWebContentsObserver;

// MediaPlayerRenderer bridges the media::Renderer and Android MediaPlayer
// interfaces. It owns a MediaPlayerBridge, which exposes c++ methods to call
// into a native Android MediaPlayer.
//
// Each MediaPlayerRenderer is associated with one MediaPlayerRendererClient,
// living in WMPI in the Renderer process.
//
// N.B: MediaPlayerRenderer implements MediaPlayerManager, since
// MediaPlayerBridge is tightly coupled with the manager abstraction.
// |player_id| is ignored in all MediaPlayerManager calls, as there is only one
// MediaPlayer per MediaPlayerRenderer.
//
// TODO(tguilbert): Remove the MediaPlayerManager implementation and update
// MediaPlayerBridge, once WMPA has been deleted. See http://crbug.com/580626
class CONTENT_EXPORT MediaPlayerRenderer : public media::Renderer,
                                           public media::MediaPlayerManager {
 public:
  // Permits embedders to handle custom urls.
  static void RegisterMediaUrlInterceptor(
      media::MediaUrlInterceptor* media_url_interceptor);

  MediaPlayerRenderer(int process_id,
                      int routing_id,
                      WebContents* web_contents);

  ~MediaPlayerRenderer() override;

  // media::Renderer implementation
  void Initialize(media::MediaResource* media_resource,
                  media::RendererClient* client,
                  const media::PipelineStatusCB& init_cb) override;
  void SetCdm(media::CdmContext* cdm_context,
              const media::CdmAttachedCB& cdm_attached_cb) override;
  void Flush(const base::Closure& flush_cb) override;
  void StartPlayingFrom(base::TimeDelta time) override;

  // N.B: MediaPlayerBridge doesn't support variable playback rates (but it
  // could be exposed from MediaPlayer in the future). For the moment:
  // - If |playback_rate| is 0, we pause the video.
  // - For other |playback_rate| values, we start playing at 1x speed.
  void SetPlaybackRate(double playback_rate) override;
  void SetVolume(float volume) override;
  base::TimeDelta GetMediaTime() override;

  // media::MediaPlayerManager implementation
  media::MediaResourceGetter* GetMediaResourceGetter() override;
  media::MediaUrlInterceptor* GetMediaUrlInterceptor() override;
  void OnTimeUpdate(int player_id,
                    base::TimeDelta current_timestamp,
                    base::TimeTicks current_time_ticks) override;
  void OnMediaMetadataChanged(int player_id,
                              base::TimeDelta duration,
                              int width,
                              int height,
                              bool success) override;
  void OnPlaybackComplete(int player_id) override;
  void OnMediaInterrupted(int player_id) override;
  void OnBufferingUpdate(int player_id, int percentage) override;
  void OnSeekComplete(int player_id,
                      const base::TimeDelta& current_time) override;
  void OnError(int player_id, int error) override;
  void OnVideoSizeChanged(int player_id, int width, int height) override;
  media::MediaPlayerAndroid* GetFullscreenPlayer() override;
  media::MediaPlayerAndroid* GetPlayer(int player_id) override;
  bool RequestPlay(int player_id,
                   base::TimeDelta duration,
                   bool has_audio) override;

  void OnUpdateAudioMutingState(bool muted);
  void OnWebContentsDestroyed();

  // Registers a request in the content::ScopedSurfaceRequestManager, and
  // returns the token associated to the request. The token can then be used to
  // complete the request via the gpu::ScopedSurfaceRequestConduit.
  // A completed request will call back to OnScopedSurfaceRequestCompleted().
  //
  // NOTE: If a request is already pending, calling this method again will
  // safely cancel the pending request before registering a new one.
  base::UnguessableToken InitiateScopedSurfaceRequest();
  void OnScopedSurfaceRequestCompleted(gl::ScopedJavaSurface surface);

 private:
  void CreateMediaPlayer(const media::MediaUrlParams& params,
                         const media::PipelineStatusCB& init_cb);

  // Used when creating |media_player_|.
  void OnDecoderResourcesReleased(int player_id);

  // Cancels the pending request started by InitiateScopedSurfaceRequest(), if
  // it exists. No-ops otherwise.
  void CancelScopedSurfaceRequest();

  void UpdateVolume();

  // Identifiers to find the RenderFrameHost that created |this|.
  // NOTE: We store these IDs rather than a RenderFrameHost* because we do not
  // know when the RenderFrameHost is destroyed.
  int render_process_id_;
  int routing_id_;

  media::RendererClient* renderer_client_;

  std::unique_ptr<media::MediaPlayerBridge> media_player_;

  // Current duration of the media.
  base::TimeDelta duration_;

  // Indicates if a serious error has been encountered by the |media_player_|.
  bool has_error_;

  gfx::Size video_size_;

  base::UnguessableToken surface_request_token_;

  std::unique_ptr<media::MediaResourceGetter> media_resource_getter_;

  bool web_contents_muted_;
  MediaPlayerRendererWebContentsObserver* web_contents_observer_;
  float volume_;

  // NOTE: Weak pointers must be invalidated before all other member variables.
  base::WeakPtrFactory<MediaPlayerRenderer> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MediaPlayerRenderer);
};

}  // namespace content

#endif  // CONTENT_BROWSER_MEDIA_ANDROID_MEDIA_PLAYER_RENDERER_H_
