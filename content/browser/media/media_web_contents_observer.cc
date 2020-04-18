// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/media_web_contents_observer.h"

#include <memory>

#include "build/build_config.h"
#include "content/browser/media/audible_metrics.h"
#include "content/browser/media/audio_stream_monitor.h"
#include "content/browser/picture_in_picture/picture_in_picture_window_controller_impl.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/common/media/media_player_delegate_messages.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "ipc/ipc_message_macros.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "services/device/public/mojom/wake_lock_context.mojom.h"
#include "third_party/blink/public/platform/web_fullscreen_video_status.h"
#include "ui/gfx/geometry/size.h"

namespace content {

namespace {

AudibleMetrics* GetAudibleMetrics() {
  static AudibleMetrics* metrics = new AudibleMetrics();
  return metrics;
}

void CheckFullscreenDetectionEnabled(WebContents* web_contents) {
#if defined(OS_ANDROID)
  DCHECK(web_contents->GetRenderViewHost()
             ->GetWebkitPreferences()
             .video_fullscreen_detection_enabled)
      << "Attempt to use method relying on fullscreen detection while "
      << "fullscreen detection is disabled.";
#else   // defined(OS_ANDROID)
  NOTREACHED() << "Attempt to use method relying on fullscreen detection, "
               << "which is only enabled on Android.";
#endif  // defined(OS_ANDROID)
}

// Returns true if |player_id| exists in |player_map|.
bool MediaPlayerEntryExists(
    const WebContentsObserver::MediaPlayerId& player_id,
    const MediaWebContentsObserver::ActiveMediaPlayerMap& player_map) {
  const auto& players = player_map.find(player_id.first);
  if (players == player_map.end())
    return false;

  return players->second.find(player_id.second) != players->second.end();
}

}  // anonymous namespace

MediaWebContentsObserver::MediaWebContentsObserver(WebContents* web_contents)
    : WebContentsObserver(web_contents),
      session_controllers_manager_(this) {}

MediaWebContentsObserver::~MediaWebContentsObserver() = default;

void MediaWebContentsObserver::WebContentsDestroyed() {
  GetAudibleMetrics()->UpdateAudibleWebContentsState(web_contents(), false);
}

void MediaWebContentsObserver::RenderFrameDeleted(
    RenderFrameHost* render_frame_host) {
  ClearWakeLocks(render_frame_host);
  session_controllers_manager_.RenderFrameDeleted(render_frame_host);

  if (fullscreen_player_ && fullscreen_player_->first == render_frame_host) {
    picture_in_picture_allowed_in_fullscreen_.reset();
    fullscreen_player_.reset();
  }
}

void MediaWebContentsObserver::MaybeUpdateAudibleState() {
  AudioStreamMonitor* audio_stream_monitor =
      web_contents_impl()->audio_stream_monitor();

  if (audio_stream_monitor->WasRecentlyAudible())
    LockAudio();
  else
    CancelAudioLock();

  GetAudibleMetrics()->UpdateAudibleWebContentsState(
      web_contents(), audio_stream_monitor->IsCurrentlyAudible());
}

bool MediaWebContentsObserver::HasActiveEffectivelyFullscreenVideo() const {
  CheckFullscreenDetectionEnabled(web_contents_impl());
  if (!web_contents()->IsFullscreen() || !fullscreen_player_)
    return false;

  // Check that the player is active.
  return MediaPlayerEntryExists(*fullscreen_player_, active_video_players_);
}

bool MediaWebContentsObserver::IsPictureInPictureAllowedForFullscreenVideo()
    const {
  DCHECK(picture_in_picture_allowed_in_fullscreen_.has_value());

  return *picture_in_picture_allowed_in_fullscreen_;
}

const base::Optional<WebContentsObserver::MediaPlayerId>&
MediaWebContentsObserver::GetFullscreenVideoMediaPlayerId() const {
  CheckFullscreenDetectionEnabled(web_contents_impl());
  return fullscreen_player_;
}

const base::Optional<WebContentsObserver::MediaPlayerId>&
MediaWebContentsObserver::GetPictureInPictureVideoMediaPlayerId() const {
  return pip_player_;
}

bool MediaWebContentsObserver::OnMessageReceived(
    const IPC::Message& msg,
    RenderFrameHost* render_frame_host) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP_WITH_PARAM(MediaWebContentsObserver, msg,
                                   render_frame_host)
    IPC_MESSAGE_HANDLER(MediaPlayerDelegateHostMsg_OnMediaDestroyed,
                        OnMediaDestroyed)
    IPC_MESSAGE_HANDLER(MediaPlayerDelegateHostMsg_OnMediaPaused, OnMediaPaused)
    IPC_MESSAGE_HANDLER(MediaPlayerDelegateHostMsg_OnMediaPlaying,
                        OnMediaPlaying)
    IPC_MESSAGE_HANDLER(MediaPlayerDelegateHostMsg_OnMutedStatusChanged,
                        OnMediaMutedStatusChanged)
    IPC_MESSAGE_HANDLER(
        MediaPlayerDelegateHostMsg_OnMediaEffectivelyFullscreenChanged,
        OnMediaEffectivelyFullscreenChanged)
    IPC_MESSAGE_HANDLER(MediaPlayerDelegateHostMsg_OnMediaSizeChanged,
                        OnMediaSizeChanged)
    IPC_MESSAGE_HANDLER(
        MediaPlayerDelegateHostMsg_OnPictureInPictureModeStarted,
        OnPictureInPictureModeStarted)
    IPC_MESSAGE_HANDLER(MediaPlayerDelegateHostMsg_OnPictureInPictureModeEnded,
                        OnPictureInPictureModeEnded)
    IPC_MESSAGE_HANDLER(
        MediaPlayerDelegateHostMsg_OnPictureInPictureSurfaceChanged,
        OnPictureInPictureSurfaceChanged)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void MediaWebContentsObserver::OnVisibilityChanged(
    content::Visibility visibility) {
  UpdateVideoLock();
}

void MediaWebContentsObserver::RequestPersistentVideo(bool value) {
  if (!fullscreen_player_)
    return;

  // The message is sent to the renderer even though the video is already the
  // fullscreen element itself. It will eventually be handled by Blink.
  RenderFrameHost* target_frame = fullscreen_player_->first;
  int delegate_id = fullscreen_player_->second;
  target_frame->Send(new MediaPlayerDelegateMsg_BecamePersistentVideo(
      target_frame->GetRoutingID(), delegate_id, value));
}

bool MediaWebContentsObserver::IsPlayerActive(
    const MediaPlayerId& player_id) const {
  if (MediaPlayerEntryExists(player_id, active_video_players_))
    return true;

  return MediaPlayerEntryExists(player_id, active_audio_players_);
}

void MediaWebContentsObserver::OnPictureInPictureWindowResize(
    const gfx::Size& window_size) {
  DCHECK(pip_player_.has_value());

  RenderFrameHost* frame = pip_player_->first;
  int delegate_id = pip_player_->second;
  frame->Send(new MediaPlayerDelegateMsg_OnPictureInPictureWindowResize(
      frame->GetRoutingID(), delegate_id, window_size));
}

void MediaWebContentsObserver::OnMediaDestroyed(
    RenderFrameHost* render_frame_host,
    int delegate_id) {
  OnMediaPaused(render_frame_host, delegate_id, true);
}

void MediaWebContentsObserver::OnMediaPaused(RenderFrameHost* render_frame_host,
                                             int delegate_id,
                                             bool reached_end_of_stream) {
  const MediaPlayerId player_id(render_frame_host, delegate_id);
  const bool removed_audio =
      RemoveMediaPlayerEntry(player_id, &active_audio_players_);
  const bool removed_video =
      RemoveMediaPlayerEntry(player_id, &active_video_players_);

  UpdateVideoLock();

  if (removed_audio || removed_video) {
    // Notify observers the player has been "paused".
    web_contents_impl()->MediaStoppedPlaying(
        WebContentsObserver::MediaPlayerInfo(removed_video, removed_audio),
        player_id,
        reached_end_of_stream
            ? WebContentsObserver::MediaStoppedReason::kReachedEndOfStream
            : WebContentsObserver::MediaStoppedReason::kUnspecified);
  }

  if (reached_end_of_stream)
    session_controllers_manager_.OnEnd(player_id);
  else
    session_controllers_manager_.OnPause(player_id);
}

void MediaWebContentsObserver::OnMediaPlaying(
    RenderFrameHost* render_frame_host,
    int delegate_id,
    bool has_video,
    bool has_audio,
    bool is_remote,
    media::MediaContentType media_content_type) {
  // Ignore the videos playing remotely and don't hold the wake lock for the
  // screen. TODO(dalecurtis): Is this correct? It means observers will not
  // receive play and pause messages.
  if (is_remote)
    return;

  const MediaPlayerId id(render_frame_host, delegate_id);
  if (has_audio)
    AddMediaPlayerEntry(id, &active_audio_players_);

  if (has_video) {
    AddMediaPlayerEntry(id, &active_video_players_);

    UpdateVideoLock();
  }

  if (!session_controllers_manager_.RequestPlay(
          id, has_audio, is_remote, media_content_type)) {
    return;
  }

  // Notify observers of the new player.
  DCHECK(has_audio || has_video);
  web_contents_impl()->MediaStartedPlaying(
      WebContentsObserver::MediaPlayerInfo(has_video, has_audio), id);
}

void MediaWebContentsObserver::OnMediaEffectivelyFullscreenChanged(
    RenderFrameHost* render_frame_host,
    int delegate_id,
    blink::WebFullscreenVideoStatus fullscreen_status) {
  const MediaPlayerId id(render_frame_host, delegate_id);

  switch (fullscreen_status) {
    case blink::WebFullscreenVideoStatus::kFullscreenAndPictureInPictureEnabled:
      fullscreen_player_ = id;
      picture_in_picture_allowed_in_fullscreen_ = true;
      break;
    case blink::WebFullscreenVideoStatus::
        kFullscreenAndPictureInPictureDisabled:
      fullscreen_player_ = id;
      picture_in_picture_allowed_in_fullscreen_ = false;
      break;
    case blink::WebFullscreenVideoStatus::kNotEffectivelyFullscreen:
      if (!fullscreen_player_ || *fullscreen_player_ != id)
        return;

      picture_in_picture_allowed_in_fullscreen_.reset();
      fullscreen_player_.reset();
      break;
  }

  bool is_fullscreen =
      (fullscreen_status !=
       blink::WebFullscreenVideoStatus::kNotEffectivelyFullscreen);
  web_contents_impl()->MediaEffectivelyFullscreenChanged(is_fullscreen);
}

void MediaWebContentsObserver::OnMediaSizeChanged(
    RenderFrameHost* render_frame_host,
    int delegate_id,
    const gfx::Size& size) {
  const MediaPlayerId id(render_frame_host, delegate_id);
  web_contents_impl()->MediaResized(size, id);
}

void MediaWebContentsObserver::OnPictureInPictureModeStarted(
    RenderFrameHost* render_frame_host,
    int delegate_id,
    const viz::SurfaceId& surface_id,
    const gfx::Size& natural_size,
    int request_id) {
  DCHECK(surface_id.is_valid());
  pip_player_ = MediaPlayerId(render_frame_host, delegate_id);

  UpdateVideoLock();

  gfx::Size window_size =
      web_contents_impl()->EnterPictureInPicture(surface_id, natural_size);

  render_frame_host->Send(
      new MediaPlayerDelegateMsg_OnPictureInPictureModeStarted_ACK(
          render_frame_host->GetRoutingID(), delegate_id, request_id,
          window_size));
}

void MediaWebContentsObserver::OnPictureInPictureModeEnded(
    RenderFrameHost* render_frame_host,
    int delegate_id,
    int request_id) {
  // TODO(mlamouri): must be a DCHECK but can't at the moment because we do not
  // correctly notify players when switching PIP video in the same tab.
  if (pip_player_) {
    web_contents_impl()->ExitPictureInPicture();

    // Reset must happen after notifying the WebContents because it may interact
    // with it.
    pip_player_.reset();

    UpdateVideoLock();
  }

  render_frame_host->Send(
      new MediaPlayerDelegateMsg_OnPictureInPictureModeEnded_ACK(
          render_frame_host->GetRoutingID(), delegate_id, request_id));
}

void MediaWebContentsObserver::OnPictureInPictureSurfaceChanged(
    RenderFrameHost* render_frame_host,
    int delegate_id,
    const viz::SurfaceId& surface_id,
    const gfx::Size& natural_size) {
  DCHECK(surface_id.is_valid());
  DCHECK(pip_player_);

  PictureInPictureWindowControllerImpl* pip_controller =
      PictureInPictureWindowControllerImpl::FromWebContents(
          web_contents_impl());
  DCHECK(pip_controller);

  pip_controller->EmbedSurface(surface_id, natural_size);
}

void MediaWebContentsObserver::ClearWakeLocks(
    RenderFrameHost* render_frame_host) {
  std::set<MediaPlayerId> video_players;
  RemoveAllMediaPlayerEntries(render_frame_host, &active_video_players_,
                              &video_players);
  std::set<MediaPlayerId> audio_players;
  RemoveAllMediaPlayerEntries(render_frame_host, &active_audio_players_,
                              &audio_players);

  std::set<MediaPlayerId> removed_players;
  std::set_union(video_players.begin(), video_players.end(),
                 audio_players.begin(), audio_players.end(),
                 std::inserter(removed_players, removed_players.end()));

  UpdateVideoLock();

  // Notify all observers the player has been "paused".
  for (const auto& id : removed_players) {
    auto it = video_players.find(id);
    bool was_video = (it != video_players.end());
    bool was_audio = (audio_players.find(id) != audio_players.end());
    web_contents_impl()->MediaStoppedPlaying(
        WebContentsObserver::MediaPlayerInfo(was_video, was_audio), id,
        WebContentsObserver::MediaStoppedReason::kUnspecified);
  }
}

device::mojom::WakeLock* MediaWebContentsObserver::GetAudioWakeLock() {
  // Here is a lazy binding, and will not reconnect after connection error.
  if (!audio_wake_lock_) {
    device::mojom::WakeLockRequest request =
        mojo::MakeRequest(&audio_wake_lock_);
    device::mojom::WakeLockContext* wake_lock_context =
        web_contents()->GetWakeLockContext();
    if (wake_lock_context) {
      wake_lock_context->GetWakeLock(
          device::mojom::WakeLockType::kPreventAppSuspension,
          device::mojom::WakeLockReason::kAudioPlayback, "Playing audio",
          std::move(request));
    }
  }
  return audio_wake_lock_.get();
}

device::mojom::WakeLock* MediaWebContentsObserver::GetVideoWakeLock() {
  // Here is a lazy binding, and will not reconnect after connection error.
  if (!video_wake_lock_) {
    device::mojom::WakeLockRequest request =
        mojo::MakeRequest(&video_wake_lock_);
    device::mojom::WakeLockContext* wake_lock_context =
        web_contents()->GetWakeLockContext();
    if (wake_lock_context) {
      wake_lock_context->GetWakeLock(
          device::mojom::WakeLockType::kPreventDisplaySleep,
          device::mojom::WakeLockReason::kVideoPlayback, "Playing video",
          std::move(request));
    }
  }
  return video_wake_lock_.get();
}

void MediaWebContentsObserver::LockAudio() {
  GetAudioWakeLock()->RequestWakeLock();
  has_audio_wake_lock_for_testing_ = true;
}

void MediaWebContentsObserver::CancelAudioLock() {
  GetAudioWakeLock()->CancelWakeLock();
  has_audio_wake_lock_for_testing_ = false;
}

void MediaWebContentsObserver::UpdateVideoLock() {
  if (active_video_players_.empty() ||
      (web_contents()->GetVisibility() == Visibility::HIDDEN &&
       !web_contents()->IsBeingCaptured() && !pip_player_.has_value())) {
    // Need to release a wake lock if one is held.
    if (!has_video_wake_lock_)
      return;

    GetVideoWakeLock()->CancelWakeLock();
    has_video_wake_lock_ = false;
    return;
  }

  // Need to take a wake lock if not already done.
  if (has_video_wake_lock_)
    return;

  GetVideoWakeLock()->RequestWakeLock();
  has_video_wake_lock_ = true;
}

void MediaWebContentsObserver::OnMediaMutedStatusChanged(
    RenderFrameHost* render_frame_host,
    int delegate_id,
    bool muted) {
  const MediaPlayerId id(render_frame_host, delegate_id);
  web_contents_impl()->MediaMutedStatusChanged(id, muted);
}

void MediaWebContentsObserver::AddMediaPlayerEntry(
    const MediaPlayerId& id,
    ActiveMediaPlayerMap* player_map) {
  (*player_map)[id.first].insert(id.second);
}

bool MediaWebContentsObserver::RemoveMediaPlayerEntry(
    const MediaPlayerId& id,
    ActiveMediaPlayerMap* player_map) {
  auto it = player_map->find(id.first);
  if (it == player_map->end())
    return false;

  // Remove the player.
  bool did_remove = it->second.erase(id.second) == 1;
  if (!did_remove)
    return false;

  // If there are no players left, remove the map entry.
  if (it->second.empty())
    player_map->erase(it);

  return true;
}

void MediaWebContentsObserver::RemoveAllMediaPlayerEntries(
    RenderFrameHost* render_frame_host,
    ActiveMediaPlayerMap* player_map,
    std::set<MediaPlayerId>* removed_players) {
  auto it = player_map->find(render_frame_host);
  if (it == player_map->end())
    return;

  for (int delegate_id : it->second)
    removed_players->insert(MediaPlayerId(render_frame_host, delegate_id));

  player_map->erase(it);
}

WebContentsImpl* MediaWebContentsObserver::web_contents_impl() const {
  return static_cast<WebContentsImpl*>(web_contents());
}

}  // namespace content
