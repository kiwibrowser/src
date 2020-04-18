// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BLINK_RENDERER_MEDIA_PLAYER_INTERFACE_H_
#define MEDIA_BLINK_RENDERER_MEDIA_PLAYER_INTERFACE_H_

// This file contains interfaces modeled after classes in
// content/renderer/media/android for the purposes of letting clases in
// this directory implement and/or interact with those classes.
// It's a stop-gap used to support cast on android until a better solution
// is implemented: crbug/575276

#include <string>
#include "base/time/time.h"
#include "media/blink/webmediaplayer_delegate.h"
#include "ui/gfx/geometry/rect_f.h"
#include "url/gurl.h"

namespace blink {
enum class WebRemotePlaybackAvailability;
}

// Dictates which type of media playback is being initialized.
enum MediaPlayerHostMsg_Initialize_Type {
  MEDIA_PLAYER_TYPE_URL,
  MEDIA_PLAYER_TYPE_REMOTE_ONLY,
  MEDIA_PLAYER_TYPE_LAST = MEDIA_PLAYER_TYPE_REMOTE_ONLY
};

namespace media {

class RendererMediaPlayerInterface {
 public:
  virtual void OnMediaMetadataChanged(base::TimeDelta duration,
                                      int width,
                                      int height,
                                      bool success) = 0;
  virtual void OnPlaybackComplete() = 0;
  virtual void OnBufferingUpdate(int percentage) = 0;
  virtual void OnSeekRequest(base::TimeDelta time_to_seek) = 0;
  virtual void OnSeekComplete(base::TimeDelta current_time) = 0;
  virtual void OnMediaError(int error_type) = 0;
  virtual void OnVideoSizeChanged(int width, int height) = 0;

  // Called to update the current time.
  virtual void OnTimeUpdate(base::TimeDelta current_timestamp,
                            base::TimeTicks current_time_ticks) = 0;

  virtual void OnPlayerReleased() = 0;

  // Functions called when media player status changes.
  virtual void OnConnectedToRemoteDevice(
      const std::string& remote_playback_message) = 0;
  virtual void OnDisconnectedFromRemoteDevice() = 0;
  virtual void OnRemotePlaybackStarted() = 0;
  virtual void OnCancelledRemotePlaybackRequest() = 0;
  virtual void OnDidExitFullscreen() = 0;
  virtual void OnMediaPlayerPlay() = 0;
  virtual void OnMediaPlayerPause() = 0;
  virtual void OnRemoteRouteAvailabilityChanged(
      blink::WebRemotePlaybackAvailability availability) = 0;

  // This function is called by the RendererMediaPlayerManager to pause the
  // video and release the media player and surface texture when we switch tabs.
  // However, the actual GlTexture is not released to keep the video screenshot.
  virtual void SuspendAndReleaseResources() = 0;
};

class RendererMediaPlayerManagerInterface {
 public:
  // Initializes a MediaPlayerAndroid object in browser process.
  virtual void Initialize(MediaPlayerHostMsg_Initialize_Type type,
                          int player_id,
                          const GURL& url,
                          const GURL& site_for_cookies,
                          const GURL& frame_url,
                          bool allow_credentials,
                          int delegate_id) = 0;

  // Starts the player.
  virtual void Start(int player_id) = 0;

  // Pauses the player.
  // is_media_related_action should be true if this pause is coming from an
  // an action that explicitly pauses the video (user pressing pause, JS, etc.)
  // Otherwise it should be false if Pause is being called due to other reasons
  // (cleanup, freeing resources, etc.)
  virtual void Pause(int player_id, bool is_media_related_action) = 0;

  // Performs seek on the player.
  virtual void Seek(int player_id, base::TimeDelta time) = 0;

  // Sets the player volume.
  virtual void SetVolume(int player_id, double volume) = 0;

  // Sets the poster image.
  virtual void SetPoster(int player_id, const GURL& poster) = 0;

  // Releases resources for the player after being suspended.
  virtual void SuspendAndReleaseResources(int player_id) = 0;

  // Destroys the player in the browser process
  virtual void DestroyPlayer(int player_id) = 0;

  // Requests remote playback if possible
  virtual void RequestRemotePlayback(int player_id) = 0;

  // Requests control of remote playback
  virtual void RequestRemotePlaybackControl(int player_id) = 0;

  // Requests stopping remote playback
  virtual void RequestRemotePlaybackStop(int player_id) = 0;

  // Registers and unregisters a RendererMediaPlayerInterface object.
  virtual int RegisterMediaPlayer(RendererMediaPlayerInterface* player) = 0;
  virtual void UnregisterMediaPlayer(int player_id) = 0;
};

}  // namespace media

#endif  // MEDIA_BLINK_RENDERER_MEDIA_PLAYER_INTERFACE_H_
