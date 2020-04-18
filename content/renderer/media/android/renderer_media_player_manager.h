// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_ANDROID_RENDERER_MEDIA_PLAYER_MANAGER_H_
#define CONTENT_RENDERER_MEDIA_ANDROID_RENDERER_MEDIA_PLAYER_MANAGER_H_

#include <map>
#include <string>

#include "base/macros.h"
#include "base/time/time.h"
#include "content/public/renderer/render_frame_observer.h"
#include "media/base/android/media_player_android.h"
#include "media/blink/renderer_media_player_interface.h"
#include "url/gurl.h"

namespace blink {
enum class WebRemotePlaybackAvailability;
}

namespace content {
class WebMediaPlayerAndroid;

// Class for managing all the WebMediaPlayerAndroid objects in the same
// RenderFrame.
class RendererMediaPlayerManager :
      public RenderFrameObserver,
      public media::RendererMediaPlayerManagerInterface {
 public:
  // Constructs a RendererMediaPlayerManager object for the |render_frame|.
  explicit RendererMediaPlayerManager(RenderFrame* render_frame);
  ~RendererMediaPlayerManager() override;

  // RenderFrameObserver overrides.
  bool OnMessageReceived(const IPC::Message& msg) override;

  // Initializes a MediaPlayerAndroid object in browser process.
  void Initialize(MediaPlayerHostMsg_Initialize_Type type,
                  int player_id,
                  const GURL& url,
                  const GURL& site_for_cookies,
                  const GURL& frame_url,
                  bool allow_credentials,
                  int delegate_id) override;

  // Starts the player.
  void Start(int player_id) override;

  // Pauses the player.
  // is_media_related_action should be true if this pause is coming from an
  // an action that explicitly pauses the video (user pressing pause, JS, etc.)
  // Otherwise it should be false if Pause is being called due to other reasons
  // (cleanup, freeing resources, etc.)
  void Pause(int player_id, bool is_media_related_action) override;

  // Performs seek on the player.
  void Seek(int player_id, base::TimeDelta time) override;

  // Sets the player volume.
  void SetVolume(int player_id, double volume) override;

  // Sets the poster image.
  void SetPoster(int player_id, const GURL& poster) override;

  // Releases resources for the player after being suspended.
  void SuspendAndReleaseResources(int player_id) override;

  // Destroys the player in the browser process
  void DestroyPlayer(int player_id) override;

  // Requests remote playback if possible
  void RequestRemotePlayback(int player_id) override;

  // Requests control of remote playback
  void RequestRemotePlaybackControl(int player_id) override;

  // Requests stopping remote playback
  void RequestRemotePlaybackStop(int player_id) override;

  // Requests the player to enter fullscreen.
  void EnterFullscreen(int player_id);

  // Registers and unregisters a WebMediaPlayerAndroid object.
  int RegisterMediaPlayer(media::RendererMediaPlayerInterface* player) override;
  void UnregisterMediaPlayer(int player_id) override;

  // Gets the pointer to WebMediaPlayerAndroid given the |player_id|.
  media::RendererMediaPlayerInterface* GetMediaPlayer(int player_id);

 private:
  // RenderFrameObserver implementation.
  void OnDestruct() override;

  // Message handlers.
  void OnMediaMetadataChanged(int player_id,
                              base::TimeDelta duration,
                              int width,
                              int height,
                              bool success);
  void OnMediaPlaybackCompleted(int player_id);
  void OnMediaBufferingUpdate(int player_id, int percent);
  void OnSeekRequest(int player_id, base::TimeDelta time_to_seek);
  void OnSeekCompleted(int player_id, base::TimeDelta current_timestamp);
  void OnMediaError(int player_id, int error);
  void OnVideoSizeChanged(int player_id, int width, int height);
  void OnTimeUpdate(int player_id,
                    base::TimeDelta current_timestamp,
                    base::TimeTicks current_time_ticks);
  void OnMediaPlayerReleased(int player_id);
  void OnConnectedToRemoteDevice(int player_id,
      const std::string& remote_playback_message);
  void OnDisconnectedFromRemoteDevice(int player_id);
  void OnCancelledRemotePlaybackRequest(int player_id);
  void OnRemotePlaybackStarted(int player_id);
  void OnDidExitFullscreen(int player_id);
  void OnDidEnterFullscreen(int player_id);
  void OnPlayerPlay(int player_id);
  void OnPlayerPause(int player_id);
  void OnRemoteRouteAvailabilityChanged(
      int player_id, blink::WebRemotePlaybackAvailability availability);

  // Info for all available WebMediaPlayerAndroid on a page; kept so that
  // we can enumerate them to send updates about tab focus and visibility.
  std::map<int, media::RendererMediaPlayerInterface*> media_players_;

  int next_media_player_id_;

  DISALLOW_COPY_AND_ASSIGN(RendererMediaPlayerManager);
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_ANDROID_RENDERER_MEDIA_PLAYER_MANAGER_H_
