// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Delete this file when WMPI_CAST is no longer needed.

#ifndef MEDIA_BLINK_WEBMEDIAPLAYER_CAST_ANDROID_H_
#define MEDIA_BLINK_WEBMEDIAPLAYER_CAST_ANDROID_H_

#include "base/memory/weak_ptr.h"
#include "media/blink/media_blink_export.h"
#include "media/blink/renderer_media_player_interface.h"
#include "media/blink/webmediaplayer_params.h"
#include "url/gurl.h"

namespace blink {
class WebLocalFrame;
class WebMediaPlayerClient;
class WebURL;
}

namespace media {

class VideoFrame;
class WebMediaPlayerImpl;

// This shim allows WebMediaPlayer to act sufficiently similar to
// WebMediaPlayerAndroid (by extending RendererMediaPlayerInterface)
// to implement cast functionality.
class WebMediaPlayerCast : public RendererMediaPlayerInterface {
 public:
  WebMediaPlayerCast(WebMediaPlayerImpl* impl,
                     blink::WebMediaPlayerClient* client,
                     scoped_refptr<viz::ContextProvider> context_provider);
  ~WebMediaPlayerCast();

  void Initialize(const GURL& url,
                  blink::WebLocalFrame* frame,
                  int delegate_id);

  void requestRemotePlayback();
  void requestRemotePlaybackControl();
  void requestRemotePlaybackStop();

  void SetMediaPlayerManager(
      RendererMediaPlayerManagerInterface* media_player_manager);
  bool isRemote() const { return is_remote_; }
  bool IsPaused() const { return paused_; }

  base::TimeDelta currentTime() const;
  void play();
  void pause();
  void seek(base::TimeDelta t);

  // RendererMediaPlayerInterface implementation
  void OnMediaMetadataChanged(base::TimeDelta duration,
                              int width,
                              int height,
                              bool success) override;
  void OnPlaybackComplete() override;
  void OnBufferingUpdate(int percentage) override;
  void OnSeekRequest(base::TimeDelta time_to_seek) override;
  void OnSeekComplete(base::TimeDelta current_time) override;
  void OnMediaError(int error_type) override;
  void OnVideoSizeChanged(int width, int height) override;

  // Called to update the current time.
  void OnTimeUpdate(base::TimeDelta current_timestamp,
                    base::TimeTicks current_time_ticks) override;

  // void OnWaitingForDecryptionKey() override;
  void OnPlayerReleased() override;

  // Functions called when media player status changes.
  void OnConnectedToRemoteDevice(
      const std::string& remote_playback_message) override;
  void OnDisconnectedFromRemoteDevice() override;
  void OnCancelledRemotePlaybackRequest() override;
  void OnRemotePlaybackStarted() override;
  void OnDidExitFullscreen() override;
  void OnMediaPlayerPlay() override;
  void OnMediaPlayerPause() override;
  void OnRemoteRouteAvailabilityChanged(
      blink::WebRemotePlaybackAvailability availability) override;

  // This function is called by the RendererMediaPlayerManager to pause the
  // video and release the media player and surface texture when we switch tabs.
  // However, the actual GlTexture is not released to keep the video screenshot.
  void SuspendAndReleaseResources() override;

  void SetDeviceScaleFactor(float scale_factor);
  scoped_refptr<VideoFrame> GetCastingBanner();
  void setPoster(const blink::WebURL& poster);

 private:
  WebMediaPlayerImpl* webmediaplayer_;
  blink::WebMediaPlayerClient* client_;
  scoped_refptr<viz::ContextProvider> context_provider_;

  // Manages this object and delegates player calls to the browser process.
  // Owned by RenderFrameImpl.
  RendererMediaPlayerManagerInterface* player_manager_ = nullptr;

  // Player ID assigned by the |player_manager_|.
  int player_id_;

  // Whether the browser is currently connected to a remote media player.
  bool is_remote_ = false;

  bool paused_ = true;
  bool initializing_ = false;
  bool should_notify_time_changed_ = false;

  // Last reported playout time.
  base::TimeDelta remote_time_;
  base::TimeTicks remote_time_at_;
  base::TimeDelta duration_;

  // Whether the media player has been initialized.
  bool is_player_initialized_ = false;

  std::string remote_playback_message_;

  float device_scale_factor_ = 1.0;

  DISALLOW_COPY_AND_ASSIGN(WebMediaPlayerCast);
};

// Make a texture-backed video of the given size containing the given message.
MEDIA_BLINK_EXPORT scoped_refptr<VideoFrame> MakeTextFrameForCast(
    const std::string& remote_playback_message,
    gfx::Size canvas_size,
    gfx::Size natural_size,
    const base::Callback<gpu::gles2::GLES2Interface*()>& context_3d_cb);

}  // namespace media

#endif  // MEDIA_BLINK_WEBMEDIAPLAYER_CAST_ANDROID_H_
