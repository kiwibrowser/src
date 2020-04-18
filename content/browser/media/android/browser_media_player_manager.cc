// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/android/browser_media_player_manager.h"

#include <utility>

#include "base/android/scoped_java_ref.h"
#include "base/memory/singleton.h"
#include "content/browser/frame_host/render_frame_host_impl.h"
#include "content/browser/media/android/media_resource_getter_impl.h"
#include "content/browser/media/android/media_web_contents_observer_android.h"
#include "content/browser/renderer_host/render_view_host_impl.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/common/media/media_player_messages_android.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/common/content_client.h"
#include "content/public/common/content_switches.h"
#include "media/base/android/media_player_bridge.h"
#include "media/base/android/media_url_interceptor.h"
#include "media/base/media_content_type.h"

#if !defined(USE_AURA)
#include "content/browser/android/content_view_core.h"
#include "content/browser/renderer_host/render_widget_host_view_android.h"
#endif

using media::MediaPlayerAndroid;
using media::MediaPlayerBridge;
using media::MediaPlayerManager;

namespace content {

// Threshold on the number of media players per renderer before we start
// attempting to release inactive media players.
const int kMediaPlayerThreshold = 1;
const int kInvalidMediaPlayerId = -1;

static BrowserMediaPlayerManager::Factory
    g_browser_media_player_manager_factory = NULL;
static media::MediaUrlInterceptor* media_url_interceptor_ = NULL;

// static
void BrowserMediaPlayerManager::RegisterFactory(Factory factory) {
  // TODO(aberent) nullptr test is a temporary fix to simplify upstreaming Cast.
  // Until Cast is fully upstreamed we want the downstream factory to take
  // priority over the upstream factory. The downstream call happens first,
  // so this will ensure that it does.
  if (g_browser_media_player_manager_factory == nullptr)
    g_browser_media_player_manager_factory = factory;
}

// static
void BrowserMediaPlayerManager::RegisterMediaUrlInterceptor(
    media::MediaUrlInterceptor* media_url_interceptor) {
  media_url_interceptor_ = media_url_interceptor;
}

// static
BrowserMediaPlayerManager* BrowserMediaPlayerManager::Create(
    RenderFrameHost* rfh) {
  // In chrome, |g_browser_media_player_manager_factory| should be set
  // to create a RemoteMediaPlayerManager, since RegisterFactory()
  // should be called from
  // ChromeMainDelegateAndroid::BasicStartupComplete.
  //
  // In webview, no factory should be set, and returning a nullptr should be
  // handled by the caller.
  return g_browser_media_player_manager_factory != nullptr
             ? g_browser_media_player_manager_factory(rfh)
             : nullptr;
}

std::unique_ptr<MediaPlayerAndroid>
BrowserMediaPlayerManager::CreateMediaPlayer(
    const MediaPlayerHostMsg_Initialize_Params& media_player_params,
    bool hide_url_log) {
  switch (media_player_params.type) {
    case MEDIA_PLAYER_TYPE_REMOTE_ONLY:
    case MEDIA_PLAYER_TYPE_URL: {
      const std::string user_agent = GetContentClient()->GetUserAgent();
      auto media_player_bridge = std::make_unique<MediaPlayerBridge>(
          media_player_params.player_id, media_player_params.url,
          media_player_params.site_for_cookies, user_agent, hide_url_log, this,
          base::Bind(&BrowserMediaPlayerManager::OnDecoderResourcesReleased,
                     weak_ptr_factory_.GetWeakPtr()),
          media_player_params.frame_url, media_player_params.allow_credentials);

      if (media_player_params.type == MEDIA_PLAYER_TYPE_REMOTE_ONLY)
        return std::move(media_player_bridge);

      bool should_block = false;
      bool extract_metadata =
          // Initialize the player will cause MediaMetadataExtractor to decode
          // small chunks of data.
          RequestDecoderResources(media_player_params.player_id, true);
#if !defined(USE_AURA)
      if (WebContentsDelegate* delegate = web_contents_->GetDelegate()) {
        should_block =
            delegate->ShouldBlockMediaRequest(media_player_params.url);
      } else {
        extract_metadata = false;
      }
#endif
      if (!extract_metadata) {
        // May reach here due to prerendering or throttling. Don't extract the
        // metadata since it is expensive.
        // TODO(qinmin): extract the metadata once the user decided to load
        // the page.
        OnMediaMetadataChanged(media_player_params.player_id, base::TimeDelta(),
                               0, 0, false);
      } else if (!should_block) {
        media_player_bridge->Initialize();
      }
      return std::move(media_player_bridge);
    }
  }

  NOTREACHED();
  return nullptr;
}

BrowserMediaPlayerManager::BrowserMediaPlayerManager(
    RenderFrameHost* render_frame_host)
    : render_frame_host_(render_frame_host),
      fullscreen_player_id_(kInvalidMediaPlayerId),
      fullscreen_player_is_released_(false),
      web_contents_(WebContents::FromRenderFrameHost(render_frame_host)),
      weak_ptr_factory_(this) {
}

BrowserMediaPlayerManager::~BrowserMediaPlayerManager() {
  // During the tear down process, OnDestroyPlayer() may or may not be called
  // (e.g. the WebContents may be destroyed before the render process). So
  // we cannot DCHECK(players_.empty()) here. Instead, all media players in
  // |players_| will be destroyed here because |player_| is a
  // std::vector<std::unique_ptr<>>.

  for (auto& player : players_)
    player.release()->DeleteOnCorrectThread();

  players_.clear();
}

void BrowserMediaPlayerManager::DidExitFullscreen(bool release_media_player) {
#if defined(USE_AURA)
  // TODO(crbug.com/548024)
  NOTIMPLEMENTED();
#else
  if (WebContentsDelegate* delegate = web_contents_->GetDelegate())
    delegate->ExitFullscreenModeForTab(web_contents_);

  Send(
      new MediaPlayerMsg_DidExitFullscreen(RoutingID(), fullscreen_player_id_));
  video_view_.reset();
  MediaPlayerAndroid* player = GetFullscreenPlayer();
  fullscreen_player_id_ = kInvalidMediaPlayerId;
  if (!player)
    return;

  if (release_media_player)
    ReleaseFullscreenPlayer(player);
  else
    player->SetVideoSurface(gl::ScopedJavaSurface());
#endif  // defined(USE_AURA)
}

void BrowserMediaPlayerManager::OnTimeUpdate(
    int player_id,
    base::TimeDelta current_timestamp,
    base::TimeTicks current_time_ticks) {
  Send(new MediaPlayerMsg_MediaTimeUpdate(
      RoutingID(), player_id, current_timestamp, current_time_ticks));
}

void BrowserMediaPlayerManager::SetVideoSurface(gl::ScopedJavaSurface surface) {
  MediaPlayerAndroid* player = GetFullscreenPlayer();
  if (!player)
    return;

  bool empty_surface = surface.IsEmpty();
  player->SetVideoSurface(std::move(surface));
  if (empty_surface)
    return;

  // If we already know the size, set it now. Otherwise it will be set when the
  // player gets it.
  if (player->IsPlayerReady()) {
    video_view_->OnVideoSizeChanged(player->GetVideoWidth(),
                                    player->GetVideoHeight());
  }
}

void BrowserMediaPlayerManager::OnMediaMetadataChanged(
    int player_id, base::TimeDelta duration, int width, int height,
    bool success) {
  Send(new MediaPlayerMsg_MediaMetadataChanged(
      RoutingID(), player_id, duration, width, height, success));
  if (fullscreen_player_id_ == player_id)
    video_view_->OnVideoSizeChanged(width, height);
}

void BrowserMediaPlayerManager::OnPlaybackComplete(int player_id) {
  Send(new MediaPlayerMsg_MediaPlaybackCompleted(RoutingID(), player_id));
}

void BrowserMediaPlayerManager::OnMediaInterrupted(int player_id) {
  // Tell WebKit that the audio should be paused, then release all resources
  Send(new MediaPlayerMsg_MediaPlayerReleased(RoutingID(), player_id));
  ReleaseResources(player_id);
}

void BrowserMediaPlayerManager::OnBufferingUpdate(int player_id,
                                                  int percentage) {
  Send(new MediaPlayerMsg_MediaBufferingUpdate(RoutingID(), player_id,
                                               percentage));
}

void BrowserMediaPlayerManager::OnSeekRequest(
    int player_id,
    const base::TimeDelta& time_to_seek) {
  Send(new MediaPlayerMsg_SeekRequest(RoutingID(), player_id, time_to_seek));
}

void BrowserMediaPlayerManager::OnSeekComplete(
    int player_id,
    const base::TimeDelta& current_time) {
  Send(new MediaPlayerMsg_SeekCompleted(RoutingID(), player_id, current_time));
}

void BrowserMediaPlayerManager::OnError(int player_id, int error) {
  Send(new MediaPlayerMsg_MediaError(RoutingID(), player_id, error));
  if (fullscreen_player_id_ == player_id &&
      error != MediaPlayerAndroid::MEDIA_ERROR_INVALID_CODE) {
    video_view_->OnMediaPlayerError(error);
  }
}

void BrowserMediaPlayerManager::OnVideoSizeChanged(
    int player_id, int width, int height) {
  Send(new MediaPlayerMsg_MediaVideoSizeChanged(RoutingID(), player_id,
      width, height));
  if (fullscreen_player_id_ == player_id)
    video_view_->OnVideoSizeChanged(width, height);
}

media::MediaResourceGetter*
BrowserMediaPlayerManager::GetMediaResourceGetter() {
  if (!media_resource_getter_.get()) {
    RenderProcessHost* host = web_contents()->GetMainFrame()->GetProcess();
    BrowserContext* context = host->GetBrowserContext();
    StoragePartition* partition = host->GetStoragePartition();
    storage::FileSystemContext* file_system_context =
        partition ? partition->GetFileSystemContext() : NULL;
    // Eventually this needs to be fixed to pass the correct frame rather
    // than just using the main frame.
    media_resource_getter_.reset(new MediaResourceGetterImpl(
        context,
        file_system_context,
        host->GetID(),
        web_contents()->GetMainFrame()->GetRoutingID()));
  }
  return media_resource_getter_.get();
}

media::MediaUrlInterceptor*
BrowserMediaPlayerManager::GetMediaUrlInterceptor() {
  return media_url_interceptor_;
}

MediaPlayerAndroid* BrowserMediaPlayerManager::GetFullscreenPlayer() {
  return GetPlayer(fullscreen_player_id_);
}

MediaPlayerAndroid* BrowserMediaPlayerManager::GetPlayer(int player_id) {
  for (const auto& player : players_) {
    if (player->player_id() == player_id)
      return player.get();
  }
  return nullptr;
}

bool BrowserMediaPlayerManager::RequestPlay(int player_id,
                                            base::TimeDelta duration,
                                            bool has_audio) {
  DCHECK(player_id_to_delegate_id_map_.find(player_id) !=
         player_id_to_delegate_id_map_.end());
  return MediaWebContentsObserverAndroid::FromWebContents(web_contents_)
      ->RequestPlay(render_frame_host_,
                    player_id_to_delegate_id_map_[player_id], has_audio,
                    IsPlayingRemotely(player_id),
                    media::DurationToMediaContentType(duration));
}

void BrowserMediaPlayerManager::OnEnterFullscreen(int player_id) {
#if defined(USE_AURA)
  // TODO(crbug.com/548024)
  NOTIMPLEMENTED();
#else
  DCHECK_EQ(fullscreen_player_id_, kInvalidMediaPlayerId);
  if (video_view_) {
    fullscreen_player_id_ = player_id;
    video_view_->OpenVideo();
    return;
  }

  if (ContentVideoView::GetInstance()) {
    // In Android WebView, two ContentViewCores could both try to enter
    // fullscreen video, we just ignore the second one.
    Send(new MediaPlayerMsg_DidExitFullscreen(RoutingID(), player_id));
    return;
  }

  // There's no ContentVideoView instance so create one.
  // If we know the video frame size, use it.
  gfx::Size natural_video_size;
  MediaPlayerAndroid* player = GetFullscreenPlayer();
  if (player && player->IsPlayerReady()) {
    natural_video_size =
        gfx::Size(player->GetVideoWidth(), player->GetVideoHeight());
  }

  if (!web_contents()->GetDelegate())
    return;

  base::android::ScopedJavaLocalRef<jobject> embedder(
      web_contents()->GetDelegate()->GetContentVideoViewEmbedder());
  video_view_.reset(
      new ContentVideoView(this, web_contents(), embedder, natural_video_size));

  base::android::ScopedJavaLocalRef<jobject> j_content_video_view =
      video_view_->GetJavaObject(base::android::AttachCurrentThread());
  if (!j_content_video_view.is_null()) {
    fullscreen_player_id_ = player_id;
  } else {
    Send(new MediaPlayerMsg_DidExitFullscreen(RoutingID(), player_id));
    video_view_.reset();
  }
#endif  // defined(USE_AURA)
}

void BrowserMediaPlayerManager::OnInitialize(
    const MediaPlayerHostMsg_Initialize_Params& media_player_params) {
  DestroyPlayer(media_player_params.player_id);

  bool is_off_the_record =
      web_contents()->GetBrowserContext()->IsOffTheRecord();
  auto player = CreateMediaPlayer(media_player_params, is_off_the_record);
  if (!player)
    return;

  AddPlayer(std::move(player), media_player_params.delegate_id);
}

void BrowserMediaPlayerManager::OnStart(int player_id) {
  MediaPlayerAndroid* player = GetPlayer(player_id);
  if (!player)
    return;

  RequestDecoderResources(player_id, false);

  player->Start();
  if (fullscreen_player_id_ == player_id && fullscreen_player_is_released_) {
    video_view_->OpenVideo();
    fullscreen_player_is_released_ = false;
  }
}

void BrowserMediaPlayerManager::OnSeek(
    int player_id,
    const base::TimeDelta& time) {
  MediaPlayerAndroid* player = GetPlayer(player_id);
  if (player)
    player->SeekTo(time);
}

void BrowserMediaPlayerManager::OnPause(
    int player_id,
    bool is_media_related_action) {
  MediaPlayerAndroid* player = GetPlayer(player_id);
  if (player)
    player->Pause(is_media_related_action);
}

void BrowserMediaPlayerManager::OnSetVolume(int player_id, double volume) {
  MediaPlayerAndroid* player = GetPlayer(player_id);
  if (player)
    player->SetVolume(volume);
}

void BrowserMediaPlayerManager::OnSetPoster(int player_id, const GURL& url) {
  // To be overridden by subclasses.
}

void BrowserMediaPlayerManager::OnSuspendAndReleaseResources(int player_id) {
  ReleaseResources(player_id);
}

void BrowserMediaPlayerManager::OnDestroyPlayer(int player_id) {
  DestroyPlayer(player_id);
  if (fullscreen_player_id_ == player_id)
    fullscreen_player_id_ = kInvalidMediaPlayerId;
}

void BrowserMediaPlayerManager::OnRequestRemotePlayback(int /* player_id */) {
  // Does nothing if we don't have a remote player
}

void BrowserMediaPlayerManager::OnRequestRemotePlaybackControl(
    int /* player_id */) {
  // Does nothing if we don't have a remote player
}

void BrowserMediaPlayerManager::OnRequestRemotePlaybackStop(
    int /* player_id */) {
  // Does nothing if we don't have a remote player
}

bool BrowserMediaPlayerManager::IsPlayingRemotely(int player_id) {
  return false;
}

void BrowserMediaPlayerManager::AddPlayer(
    std::unique_ptr<MediaPlayerAndroid> player,
    int delegate_id) {
  DCHECK(!GetPlayer(player->player_id()));
  players_.push_back(std::move(player));
  player_id_to_delegate_id_map_[players_.back()->player_id()] = delegate_id;
}

void BrowserMediaPlayerManager::DestroyPlayer(int player_id) {
  for (auto it = players_.begin(); it != players_.end(); ++it) {
    if ((*it)->player_id() == player_id) {
      it->release()->DeleteOnCorrectThread();
      players_.erase(it);
      break;
    }
  }
  active_players_.erase(player_id);
  player_id_to_delegate_id_map_.erase(player_id);
}

void BrowserMediaPlayerManager::ReleaseResources(int player_id) {
  MediaPlayerAndroid* player = GetPlayer(player_id);
  if (player)
    ReleasePlayer(player);
  if (player_id == fullscreen_player_id_)
    fullscreen_player_is_released_ = true;
}

std::unique_ptr<MediaPlayerAndroid> BrowserMediaPlayerManager::SwapPlayer(
    int player_id,
    std::unique_ptr<MediaPlayerAndroid> player) {
  std::unique_ptr<MediaPlayerAndroid> previous_player;
  for (auto it = players_.begin(); it != players_.end(); ++it) {
    if ((*it)->player_id() == player_id) {
      previous_player = std::move(*it);
      MediaWebContentsObserverAndroid::FromWebContents(web_contents_)
          ->DisconnectMediaSession(render_frame_host_,
                                   player_id_to_delegate_id_map_[player_id]);
      players_.erase(it);
      players_.push_back(std::move(player));
      break;
    }
  }
  return previous_player;
}

bool BrowserMediaPlayerManager::RequestDecoderResources(
    int player_id, bool temporary) {
  ActivePlayerMap::iterator it;
  // The player is already active, ignore it. A long running player should not
  // request temporary permissions.
  if ((it = active_players_.find(player_id)) != active_players_.end()) {
    DCHECK(!temporary || it->second);
    return true;
  }

  if (!temporary) {
    int long_running_player = 0;
    for (it = active_players_.begin(); it != active_players_.end(); ++it) {
      if (!it->second)
        long_running_player++;
    }

    // Number of active players are less than the threshold, do nothing.
    if (long_running_player < kMediaPlayerThreshold)
      return true;

    for (it = active_players_.begin(); it != active_players_.end(); ++it) {
      if (!it->second && !GetPlayer(it->first)->IsPlaying() &&
          fullscreen_player_id_ != it->first) {
        ReleasePlayer(GetPlayer(it->first));
        Send(new MediaPlayerMsg_MediaPlayerReleased(RoutingID(),
                                                    (it->first)));
      }
    }
  }

  active_players_[player_id] = temporary;
  return true;
}

void BrowserMediaPlayerManager::OnDecoderResourcesReleased(int player_id) {
  if (active_players_.find(player_id) == active_players_.end())
    return;

  active_players_.erase(player_id);
}

int BrowserMediaPlayerManager::RoutingID() {
  return render_frame_host_->GetRoutingID();
}

bool BrowserMediaPlayerManager::Send(IPC::Message* msg) {
  return render_frame_host_->Send(msg);
}

void BrowserMediaPlayerManager::ReleaseFullscreenPlayer(
    MediaPlayerAndroid* player) {
  ReleasePlayer(player);
}

void BrowserMediaPlayerManager::ReleasePlayer(MediaPlayerAndroid* player) {
  player->Release();
}

}  // namespace content
