// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/session/media_session_controller.h"

#include "content/browser/media/media_web_contents_observer.h"
#include "content/browser/media/session/media_session_impl.h"
#include "content/common/media/media_player_delegate_messages.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "media/base/media_content_type.h"

namespace content {

MediaSessionController::MediaSessionController(
    const WebContentsObserver::MediaPlayerId& id,
    MediaWebContentsObserver* media_web_contents_observer)
    : id_(id),
      media_web_contents_observer_(media_web_contents_observer),
      media_session_(
          MediaSessionImpl::Get(media_web_contents_observer_->web_contents())) {
}

MediaSessionController::~MediaSessionController() {
  if (!has_session_)
    return;
  media_session_->RemovePlayer(this, player_id_);
}

bool MediaSessionController::Initialize(
    bool has_audio,
    bool is_remote,
    media::MediaContentType media_content_type) {
  // Don't generate a new id if one has already been set.
  if (!has_session_) {
    // These objects are only created on the UI thread, so this is safe.
    DCHECK_CURRENTLY_ON(BrowserThread::UI);
    static uint32_t player_id = 0;
    player_id_ = static_cast<int>(player_id++);
  } else {
    // WebMediaPlayerAndroid does not have an accurate sense of audio presence,
    // only the MediaPlayerManager does, so WMPA never reports audio unless it's
    // sure (no video stream).  This leads to issues when Initialize() is called
    // by WMPA (reporting no audio and subsequently releasing the session) after
    // the manager accurately reported audio.
    //
    // To workaround this, |has_audio| is sticky; I.e., once a session has been
    // created with audio all future sessions will also have audio.
    //
    // TODO(dalecurtis): Delete sticky audio once we're no longer using WMPA and
    // the BrowserMediaPlayerManagers.  Tracked by http://crbug.com/580626
    has_audio = true;
  }

  // Don't bother with a MediaSession for remote players or without audio.  If
  // we already have a session from a previous call, release it.
  if (!has_audio || is_remote) {
    if (has_session_) {
      has_session_ = false;
      media_session_->RemovePlayer(this, player_id_);
    }
    return true;
  }

  // If a session can't be created, force a pause immediately.  Attempt to add a
  // session even if we already have one.  MediaSession expects AddPlayer() to
  // be called after OnPlaybackPaused() to reactivate the session.
  if (!media_session_->AddPlayer(this, player_id_, media_content_type)) {
    OnSuspend(player_id_);
    return false;
  }

  has_session_ = true;
  return true;
}

void MediaSessionController::OnSuspend(int player_id) {
  DCHECK_EQ(player_id_, player_id);
  id_.first->Send(
      new MediaPlayerDelegateMsg_Pause(id_.first->GetRoutingID(), id_.second));
}

void MediaSessionController::OnResume(int player_id) {
  DCHECK_EQ(player_id_, player_id);
  id_.first->Send(
      new MediaPlayerDelegateMsg_Play(id_.first->GetRoutingID(), id_.second));
}

void MediaSessionController::OnSeekForward(int player_id,
                                           base::TimeDelta seek_time) {
  DCHECK_EQ(player_id_, player_id);
  id_.first->Send(new MediaPlayerDelegateMsg_SeekForward(
      id_.first->GetRoutingID(), id_.second, seek_time));
}

void MediaSessionController::OnSeekBackward(int player_id,
                                            base::TimeDelta seek_time) {
  DCHECK_EQ(player_id_, player_id);
  id_.first->Send(new MediaPlayerDelegateMsg_SeekBackward(
      id_.first->GetRoutingID(), id_.second, seek_time));
}

void MediaSessionController::OnSetVolumeMultiplier(int player_id,
                                                   double volume_multiplier) {
  DCHECK_EQ(player_id_, player_id);
  id_.first->Send(new MediaPlayerDelegateMsg_UpdateVolumeMultiplier(
      id_.first->GetRoutingID(), id_.second, volume_multiplier));
}

RenderFrameHost* MediaSessionController::render_frame_host() const {
  return id_.first;
}

void MediaSessionController::OnPlaybackPaused() {
  // We check for suspension here since the renderer may issue its own pause
  // in response to or while a pause from the browser is in flight.
  if (media_session_->IsActive())
    media_session_->OnPlayerPaused(this, player_id_);
}

}  // namespace content
