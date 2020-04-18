// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/android/media_web_contents_observer_android.h"

#include <utility>

#include "base/memory/ptr_util.h"
#include "content/browser/media/android/browser_media_player_manager.h"
#include "content/browser/media/android/browser_surface_view_manager.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/common/media/media_player_delegate_messages.h"
#include "content/common/media/media_player_messages_android.h"
#include "content/common/media/surface_view_manager_messages_android.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "ipc/ipc_message_macros.h"
#include "media/base/android/media_player_android.h"

namespace content {

static void SuspendAllMediaPlayersInRenderFrame(
    RenderFrameHost* render_frame_host) {
  render_frame_host->Send(new MediaPlayerDelegateMsg_SuspendAllMediaPlayers(
      render_frame_host->GetRoutingID()));
}

MediaWebContentsObserverAndroid::MediaWebContentsObserverAndroid(
    WebContents* web_contents)
    : MediaWebContentsObserver(web_contents) {}

MediaWebContentsObserverAndroid::~MediaWebContentsObserverAndroid() {}

// static
MediaWebContentsObserverAndroid*
MediaWebContentsObserverAndroid::FromWebContents(WebContents* web_contents) {
  return static_cast<MediaWebContentsObserverAndroid*>(
      static_cast<WebContentsImpl*>(web_contents)
          ->media_web_contents_observer());
}

BrowserMediaPlayerManager*
MediaWebContentsObserverAndroid::GetMediaPlayerManager(
    RenderFrameHost* render_frame_host) {
  auto it = media_player_managers_.find(render_frame_host);
  if (it != media_player_managers_.end())
    return it->second.get();

  BrowserMediaPlayerManager* manager =
      BrowserMediaPlayerManager::Create(render_frame_host);
  media_player_managers_[render_frame_host] = base::WrapUnique(manager);
  return manager;
}

BrowserSurfaceViewManager*
MediaWebContentsObserverAndroid::GetSurfaceViewManager(
    RenderFrameHost* render_frame_host) {
  auto it = surface_view_managers_.find(render_frame_host);
  if (it != surface_view_managers_.end())
    return it->second.get();

  BrowserSurfaceViewManager* manager =
      new BrowserSurfaceViewManager(render_frame_host);
  surface_view_managers_[render_frame_host] = base::WrapUnique(manager);
  return manager;
}

void MediaWebContentsObserverAndroid::SuspendAllMediaPlayers() {
  web_contents()->ForEachFrame(
      base::BindRepeating(&SuspendAllMediaPlayersInRenderFrame));
}

bool MediaWebContentsObserverAndroid::RequestPlay(
    RenderFrameHost* render_frame_host,
    int delegate_id,
    bool has_audio,
    bool is_remote,
    media::MediaContentType media_content_type) {
  return session_controllers_manager()->RequestPlay(
      MediaPlayerId(render_frame_host, delegate_id), has_audio, is_remote,
      media_content_type);
}

void MediaWebContentsObserverAndroid::DisconnectMediaSession(
    RenderFrameHost* render_frame_host,
    int delegate_id) {
  session_controllers_manager()->OnEnd(
      MediaPlayerId(render_frame_host, delegate_id));
}

void MediaWebContentsObserverAndroid::RenderFrameDeleted(
    RenderFrameHost* render_frame_host) {
  MediaWebContentsObserver::RenderFrameDeleted(render_frame_host);

  media_player_managers_.erase(render_frame_host);
  surface_view_managers_.erase(render_frame_host);
}

bool MediaWebContentsObserverAndroid::OnMessageReceived(
    const IPC::Message& msg,
    RenderFrameHost* render_frame_host) {
  if (MediaWebContentsObserver::OnMessageReceived(msg, render_frame_host))
    return true;

  if (OnMediaPlayerMessageReceived(msg, render_frame_host))
    return true;

  if (OnSurfaceViewManagerMessageReceived(msg, render_frame_host))
    return true;

  return false;
}

bool MediaWebContentsObserverAndroid::OnMediaPlayerMessageReceived(
    const IPC::Message& msg,
    RenderFrameHost* render_frame_host) {
  // The only BMPM instance that is still currently used is the
  // RemoteMediaPlayerManager, used in casting.
  //
  // In the webview case, casting is not supported, and GetMediaPlayerManager()
  // will return a nullptr. It is safe to not handle the messages, since the
  // only message we can receive is an unavoidable MediaPlayerHostMsg_Initialize
  // that WMPI sends out in WebMediaPlayerImpl::DoLoad().
  BrowserMediaPlayerManager* media_player_manager =
      GetMediaPlayerManager(render_frame_host);

  if (!media_player_manager)
    return false;

  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(MediaWebContentsObserverAndroid, msg)
    IPC_MESSAGE_FORWARD(MediaPlayerHostMsg_EnterFullscreen,
                        media_player_manager,
                        BrowserMediaPlayerManager::OnEnterFullscreen)
    IPC_MESSAGE_FORWARD(MediaPlayerHostMsg_Initialize, media_player_manager,
                        BrowserMediaPlayerManager::OnInitialize)
    IPC_MESSAGE_FORWARD(MediaPlayerHostMsg_Start, media_player_manager,
                        BrowserMediaPlayerManager::OnStart)
    IPC_MESSAGE_FORWARD(MediaPlayerHostMsg_Seek, media_player_manager,
                        BrowserMediaPlayerManager::OnSeek)
    IPC_MESSAGE_FORWARD(MediaPlayerHostMsg_Pause, media_player_manager,
                        BrowserMediaPlayerManager::OnPause)
    IPC_MESSAGE_FORWARD(MediaPlayerHostMsg_SetVolume, media_player_manager,
                        BrowserMediaPlayerManager::OnSetVolume)
    IPC_MESSAGE_FORWARD(MediaPlayerHostMsg_SetPoster, media_player_manager,
                        BrowserMediaPlayerManager::OnSetPoster)
    IPC_MESSAGE_FORWARD(MediaPlayerHostMsg_SuspendAndRelease,
                        media_player_manager,
                        BrowserMediaPlayerManager::OnSuspendAndReleaseResources)
    IPC_MESSAGE_FORWARD(MediaPlayerHostMsg_DestroyMediaPlayer,
                        media_player_manager,
                        BrowserMediaPlayerManager::OnDestroyPlayer)
    IPC_MESSAGE_FORWARD(MediaPlayerHostMsg_RequestRemotePlayback,
                        media_player_manager,
                        BrowserMediaPlayerManager::OnRequestRemotePlayback)
    IPC_MESSAGE_FORWARD(
        MediaPlayerHostMsg_RequestRemotePlaybackControl, media_player_manager,
        BrowserMediaPlayerManager::OnRequestRemotePlaybackControl)
    IPC_MESSAGE_FORWARD(MediaPlayerHostMsg_RequestRemotePlaybackStop,
                        media_player_manager,
                        BrowserMediaPlayerManager::OnRequestRemotePlaybackStop)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

bool MediaWebContentsObserverAndroid::OnSurfaceViewManagerMessageReceived(
    const IPC::Message& msg,
    RenderFrameHost* render_frame_host) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(MediaWebContentsObserverAndroid, msg)
    IPC_MESSAGE_FORWARD(SurfaceViewManagerHostMsg_CreateFullscreenSurface,
                        GetSurfaceViewManager(render_frame_host),
                        BrowserSurfaceViewManager::OnCreateFullscreenSurface)
    IPC_MESSAGE_FORWARD(SurfaceViewManagerHostMsg_NaturalSizeChanged,
                        GetSurfaceViewManager(render_frame_host),
                        BrowserSurfaceViewManager::OnNaturalSizeChanged)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

}  // namespace content
