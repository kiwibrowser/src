// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ANDROID_REMOTE_REMOTE_MEDIA_PLAYER_MANAGER_H_
#define CHROME_BROWSER_MEDIA_ANDROID_REMOTE_REMOTE_MEDIA_PLAYER_MANAGER_H_

#include <memory>
#include <set>
#include <unordered_map>
#include <vector>

#include "base/macros.h"
#include "chrome/browser/media/android/remote/remote_media_player_bridge.h"
#include "content/browser/media/android/browser_media_player_manager.h"
#include "media/base/android/media_player_android.h"

struct MediaPlayerHostMsg_Initialize_Params;

namespace blink {
enum class WebRemotePlaybackAvailability;
}

namespace remote_media {

// media::MediaPlayerManager implementation that allows the user to play media
// remotely.
class RemoteMediaPlayerManager : public content::BrowserMediaPlayerManager {
 public:
  explicit RemoteMediaPlayerManager(
      content::RenderFrameHost* render_frame_host);
  ~RemoteMediaPlayerManager() override;

  void OnPlaying(int player_id);
  void OnPaused(int player_id);

  // Callback to trigger when a remote device has been unselected.
  void OnRemoteDeviceUnselected(int player_id);

  // Callback to trigger when the video on a remote device starts playing.
  void OnRemotePlaybackStarted(int player_id);

  // Callback to trigger when the video on a remote device finishes playing.
  void OnRemotePlaybackFinished(int player_id);

  // Callback to trigger when the availability of remote routes changes.
  void OnRouteAvailabilityChanged(
      int player_id, blink::WebRemotePlaybackAvailability availability);

  // Callback to trigger when the device picker dialog was dismissed.
  void OnCancelledRemotePlaybackRequest(int player_id);

  void OnMediaMetadataChanged(int player_id,
                              base::TimeDelta duration,
                              int width,
                              int height,
                              bool success) override;

  // Swap which player is currently in use (local or remote).
  void SwitchToRemotePlayer(int player_id, const std::string& casting_message);
  void SwitchToLocalPlayer(int player_id);

  // Get the local player for a given player id, whether or not it is currently
  // playing locally. Will return nullptr if the local player no longer exists.
  media::MediaPlayerAndroid* GetLocalPlayer(int player_id);

 protected:
  void OnSetPoster(int player_id, const GURL& url) override;

  void ReleaseResources(int player_id) override;

 private:
  // Returns a MediaPlayerAndroid implementation for playing the media remotely.
  RemoteMediaPlayerBridge* CreateRemoteMediaPlayer(int player_id);

  // Replaces the remote player with the local player this class is holding.
  // Does nothing if there is no remote player.
  void ReplaceRemotePlayerWithLocal(int player_id);

  // content::BrowserMediaPlayerManager overrides.
  void OnStart(int player_id) override;
  void OnInitialize(
      const MediaPlayerHostMsg_Initialize_Params& media_player_params) override;
  void OnDestroyPlayer(int player_id) override;
  void OnSuspendAndReleaseResources(int player_id) override;
  void OnRequestRemotePlayback(int player_id) override;
  void OnRequestRemotePlaybackControl(int player_id) override;
  void OnRequestRemotePlaybackStop(int player_id) override;

  bool IsPlayingRemotely(int player_id) override;

  void ReleaseFullscreenPlayer(media::MediaPlayerAndroid* player) override;

  // Callback for when the download of poster image is done.
  void DidDownloadPoster(
      int player_id,
      int id,
      int http_status_code,
      const GURL& image_url,
      const std::vector<SkBitmap>& bitmaps,
      const std::vector<gfx::Size>& original_bitmap_sizes);

  // Return the ID of the tab that's associated with this controller. Returns
  // -1 in case something goes wrong.
  int GetTabId();

  // Get the player that is not currently selected
  std::vector<std::unique_ptr<media::MediaPlayerAndroid>>::iterator
  GetAlternativePlayer(int player_id);

  // Get the remote player for a given player id, whether or not it is currently
  // playing remotely.
  RemoteMediaPlayerBridge* GetRemotePlayer(int player_id);

  bool SwapCurrentPlayer(int player_id);

  void FetchPosterBitmap(int player_id);

  // Contains the alternative players that are not currently in use, i.e. the
  // remote players for videos that are playing locally, and the local players
  // for videos that are playing remotely.
  std::vector<std::unique_ptr<media::MediaPlayerAndroid>> alternative_players_;

  std::set<int> players_playing_remotely_;
  std::unordered_map<int, GURL> poster_urls_;

  base::WeakPtrFactory<RemoteMediaPlayerManager> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(RemoteMediaPlayerManager);
};

} // namespace remote_media

#endif  // CHROME_BROWSER_MEDIA_ANDROID_REMOTE_REMOTE_MEDIA_PLAYER_MANAGER_H_
