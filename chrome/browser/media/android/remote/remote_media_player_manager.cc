// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/android/remote/remote_media_player_manager.h"

#include "chrome/browser/android/tab_android.h"
#include "chrome/common/chrome_content_client.h"
#include "content/common/media/media_player_messages_android.h"
#include "third_party/blink/public/platform/modules/remoteplayback/web_remote_playback_availability.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/android/java_bitmap.h"

using media::MediaPlayerAndroid;

static const int MAX_POSTER_BITMAP_SIZE = 1024 * 1024;

namespace remote_media {

RemoteMediaPlayerManager::RemoteMediaPlayerManager(
    content::RenderFrameHost* render_frame_host)
    : BrowserMediaPlayerManager(render_frame_host),
      weak_ptr_factory_(this) {
}

RemoteMediaPlayerManager::~RemoteMediaPlayerManager() {
  for (auto& player : alternative_players_)
    player.release()->DeleteOnCorrectThread();

  alternative_players_.clear();
}

void RemoteMediaPlayerManager::OnStart(int player_id) {
  RemoteMediaPlayerBridge* remote_player = GetRemotePlayer(player_id);
  if (remote_player && IsPlayingRemotely(player_id))
    remote_player->Start();

  BrowserMediaPlayerManager::OnStart(player_id);
}

void RemoteMediaPlayerManager::OnInitialize(
    const MediaPlayerHostMsg_Initialize_Params& media_params) {
  BrowserMediaPlayerManager::OnInitialize(media_params);

  if (GetPlayer(media_params.player_id)) {
    RemoteMediaPlayerBridge* remote_player =
        CreateRemoteMediaPlayer(media_params.player_id);
    remote_player->OnPlayerCreated();
  }
}

void RemoteMediaPlayerManager::OnDestroyPlayer(int player_id) {
  RemoteMediaPlayerBridge* player = GetRemotePlayer(player_id);
  if (player)
    player->OnPlayerDestroyed();
  poster_urls_.erase(player_id);
  BrowserMediaPlayerManager::OnDestroyPlayer(player_id);
}

void RemoteMediaPlayerManager::OnSuspendAndReleaseResources(int player_id) {
  // We only want to release resources of local players.
  if (!IsPlayingRemotely(player_id))
    BrowserMediaPlayerManager::OnSuspendAndReleaseResources(player_id);
}

void RemoteMediaPlayerManager::OnRequestRemotePlayback(int player_id) {
  RemoteMediaPlayerBridge* player = GetRemotePlayer(player_id);
  if (player)
    player->RequestRemotePlayback();
}

void RemoteMediaPlayerManager::OnRequestRemotePlaybackControl(int player_id) {
  RemoteMediaPlayerBridge* player = GetRemotePlayer(player_id);
  if (player)
    player->RequestRemotePlaybackControl();
}

void RemoteMediaPlayerManager::OnRequestRemotePlaybackStop(int player_id) {
  RemoteMediaPlayerBridge* player = GetRemotePlayer(player_id);
  if (player)
    player->RequestRemotePlaybackStop();
}

bool RemoteMediaPlayerManager::IsPlayingRemotely(int player_id) {
  return players_playing_remotely_.count(player_id) != 0;
}

int RemoteMediaPlayerManager::GetTabId() {
  if (!web_contents())
    return -1;

  TabAndroid* tab = TabAndroid::FromWebContents(web_contents());
  if (!tab)
    return -1;

  return tab->GetAndroidId();
}

void RemoteMediaPlayerManager::FetchPosterBitmap(int player_id) {
  RemoteMediaPlayerBridge* player = GetRemotePlayer(player_id);
  if (poster_urls_.count(player_id) == 0 ||
      poster_urls_[player_id].is_empty()) {
    if (player)
      player->SetPosterBitmap(std::vector<SkBitmap>());
    return;
  }
  content::WebContents::ImageDownloadCallback callback =
      base::BindOnce(&RemoteMediaPlayerManager::DidDownloadPoster,
                     weak_ptr_factory_.GetWeakPtr(), player_id);
  web_contents()->DownloadImage(
      poster_urls_[player_id],
      false,  // is_favicon, false so that cookies will be used.
      MAX_POSTER_BITMAP_SIZE,  // max_bitmap_size, 0 means no limit.
      false,                   // normal cache policy.
      std::move(callback));
}

void RemoteMediaPlayerManager::OnSetPoster(int player_id, const GURL& url) {
  // OnSetPoster is called when the attibutes of the video element are parsed,
  // which may be before OnInitialize is called, so we can't assume that the
  // players wil exist.
  poster_urls_[player_id] = url;
}

void RemoteMediaPlayerManager::ReleaseResources(int player_id) {
  if (IsPlayingRemotely(player_id))
    return;
  BrowserMediaPlayerManager::ReleaseResources(player_id);
}

void RemoteMediaPlayerManager::DidDownloadPoster(
    int player_id,
    int id,
    int http_status_code,
    const GURL& image_url,
    const std::vector<SkBitmap>& bitmaps,
    const std::vector<gfx::Size>& original_bitmap_sizes) {
  RemoteMediaPlayerBridge* player = GetRemotePlayer(player_id);
  if (player)
    player->SetPosterBitmap(bitmaps);
}

RemoteMediaPlayerBridge* RemoteMediaPlayerManager::CreateRemoteMediaPlayer(
    int player_id) {
  alternative_players_.push_back(std::make_unique<RemoteMediaPlayerBridge>(
      player_id, GetUserAgent(), this));
  RemoteMediaPlayerBridge* remote =
      static_cast<RemoteMediaPlayerBridge*>(alternative_players_.back().get());
  remote->Initialize();
  return remote;
}

bool RemoteMediaPlayerManager::SwapCurrentPlayer(int player_id) {
  // Find the alternative player to swap the current one with.
  auto it = GetAlternativePlayer(player_id);
  if (it == alternative_players_.end())
    return false;

  // Release ownership of the alternative player.
  std::unique_ptr<MediaPlayerAndroid> old_player =
      SwapPlayer(player_id, std::move(*it));
  alternative_players_.erase(it);
  if (!old_player) {
    return false;
  }

  alternative_players_.push_back(std::move(old_player));
  return true;
}

void RemoteMediaPlayerManager::SwitchToRemotePlayer(
    int player_id,
    const std::string& casting_message) {
  DCHECK(!IsPlayingRemotely(player_id));
  if (!SwapCurrentPlayer(player_id))
    return;
  players_playing_remotely_.insert(player_id);
  Send(new MediaPlayerMsg_DidMediaPlayerPlay(RoutingID(), player_id));
  Send(new MediaPlayerMsg_ConnectedToRemoteDevice(RoutingID(), player_id,
                                                  casting_message));
  // The remote player will want the poster bitmap, however, to avoid wasting
  // memory we don't fetch it until we are likely to need it.
  FetchPosterBitmap(player_id);
}

void RemoteMediaPlayerManager::SwitchToLocalPlayer(int player_id) {
  DCHECK(IsPlayingRemotely(player_id));
  SwapCurrentPlayer(player_id);
  players_playing_remotely_.erase(player_id);
  Send(new MediaPlayerMsg_DisconnectedFromRemoteDevice(RoutingID(), player_id));
}

void RemoteMediaPlayerManager::ReplaceRemotePlayerWithLocal(int player_id) {
  if (!IsPlayingRemotely(player_id))
    return;
  MediaPlayerAndroid* remote_player = GetPlayer(player_id);
  remote_player->Pause(true);
  Send(new MediaPlayerMsg_DidMediaPlayerPause(RoutingID(), player_id));
  Send(new MediaPlayerMsg_DisconnectedFromRemoteDevice(RoutingID(), player_id));

  GetLocalPlayer(player_id)->SeekTo(remote_player->GetCurrentTime());
  SwapCurrentPlayer(player_id);
  remote_player->Release();
  players_playing_remotely_.erase(player_id);
}

void RemoteMediaPlayerManager::OnRemoteDeviceUnselected(int player_id) {
  ReplaceRemotePlayerWithLocal(player_id);
}

void RemoteMediaPlayerManager::OnRemotePlaybackStarted(int player_id) {
  Send(new MediaPlayerMsg_RemotePlaybackStarted(RoutingID(), player_id));
}

void RemoteMediaPlayerManager::OnRemotePlaybackFinished(int player_id) {
  ReplaceRemotePlayerWithLocal(player_id);
}

void RemoteMediaPlayerManager::OnRouteAvailabilityChanged(
    int player_id, blink::WebRemotePlaybackAvailability availability) {
  Send(new MediaPlayerMsg_RemoteRouteAvailabilityChanged(
      RoutingID(), player_id, availability));
}

void RemoteMediaPlayerManager::OnCancelledRemotePlaybackRequest(int player_id) {
  Send(new MediaPlayerMsg_CancelledRemotePlaybackRequest(
      RoutingID(), player_id));
}


void RemoteMediaPlayerManager::ReleaseFullscreenPlayer(
    MediaPlayerAndroid* player) {
  int player_id = player->player_id();
  // Release the original player's resources, not the current fullscreen player
  // (which is the remote player).
  if (IsPlayingRemotely(player_id))
    GetLocalPlayer(player_id)->Release();
  else
    BrowserMediaPlayerManager::ReleaseFullscreenPlayer(player);
}

void RemoteMediaPlayerManager::OnPlaying(int player_id) {
  Send(new MediaPlayerMsg_DidMediaPlayerPlay(RoutingID(),player_id));
}

void RemoteMediaPlayerManager::OnPaused(int player_id) {
  Send(new MediaPlayerMsg_DidMediaPlayerPause(RoutingID(),player_id));
}

std::vector<std::unique_ptr<MediaPlayerAndroid>>::iterator
RemoteMediaPlayerManager::GetAlternativePlayer(int player_id) {
  for (auto it = alternative_players_.begin(); it != alternative_players_.end();
       ++it) {
    if ((*it)->player_id() == player_id) {
      return it;
    }
  }
  return alternative_players_.end();
}

RemoteMediaPlayerBridge* RemoteMediaPlayerManager::GetRemotePlayer(
    int player_id) {
  if (IsPlayingRemotely(player_id))
    return static_cast<RemoteMediaPlayerBridge*>(GetPlayer(player_id));
  auto it = GetAlternativePlayer(player_id);
  if (it == alternative_players_.end())
    return nullptr;
  return static_cast<RemoteMediaPlayerBridge*>(it->get());
}

MediaPlayerAndroid* RemoteMediaPlayerManager::GetLocalPlayer(int player_id) {
  if (!IsPlayingRemotely(player_id))
    return GetPlayer(player_id);
  auto it = GetAlternativePlayer(player_id);
  if (it == alternative_players_.end())
    return nullptr;
  return it->get();
}

void RemoteMediaPlayerManager::OnMediaMetadataChanged(int player_id,
                                                      base::TimeDelta duration,
                                                      int width,
                                                      int height,
                                                      bool success) {
  if (IsPlayingRemotely(player_id)) {
    MediaPlayerAndroid* local_player = GetLocalPlayer(player_id);
    Send(new MediaPlayerMsg_MediaMetadataChanged(
        RoutingID(), player_id, duration, local_player->GetVideoWidth(),
        local_player->GetVideoHeight(), success));
  } else {
    BrowserMediaPlayerManager::OnMediaMetadataChanged(player_id, duration,
                                                      width, height, success);
  }
}
} // namespace remote_media
