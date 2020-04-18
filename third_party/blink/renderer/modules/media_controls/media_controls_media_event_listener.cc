// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/media_controls/media_controls_media_event_listener.h"

#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/html/media/html_media_element.h"
#include "third_party/blink/renderer/core/html/track/text_track_list.h"
#include "third_party/blink/renderer/modules/media_controls/media_controls_impl.h"
#include "third_party/blink/renderer/modules/remoteplayback/availability_callback_wrapper.h"
#include "third_party/blink/renderer/modules/remoteplayback/html_media_element_remote_playback.h"
#include "third_party/blink/renderer/modules/remoteplayback/remote_playback.h"

namespace blink {

MediaControlsMediaEventListener::MediaControlsMediaEventListener(
    MediaControlsImpl* media_controls)
    : EventListener(kCPPEventListenerType), media_controls_(media_controls) {
  if (GetMediaElement().isConnected())
    Attach();
}

void MediaControlsMediaEventListener::Attach() {
  DCHECK(GetMediaElement().isConnected());

  GetMediaElement().addEventListener(EventTypeNames::volumechange, this, false);
  GetMediaElement().addEventListener(EventTypeNames::focusin, this, false);
  GetMediaElement().addEventListener(EventTypeNames::timeupdate, this, false);
  GetMediaElement().addEventListener(EventTypeNames::play, this, false);
  GetMediaElement().addEventListener(EventTypeNames::playing, this, false);
  GetMediaElement().addEventListener(EventTypeNames::pause, this, false);
  GetMediaElement().addEventListener(EventTypeNames::durationchange, this,
                                     false);
  GetMediaElement().addEventListener(EventTypeNames::error, this, false);
  GetMediaElement().addEventListener(EventTypeNames::loadedmetadata, this,
                                     false);
  GetMediaElement().addEventListener(EventTypeNames::keypress, this, false);
  GetMediaElement().addEventListener(EventTypeNames::keydown, this, false);
  GetMediaElement().addEventListener(EventTypeNames::keyup, this, false);
  GetMediaElement().addEventListener(EventTypeNames::waiting, this, false);
  GetMediaElement().addEventListener(EventTypeNames::progress, this, false);
  GetMediaElement().addEventListener(EventTypeNames::loadeddata, this, false);

  // Listen to two different fullscreen events in order to make sure the new and
  // old APIs are handled.
  GetMediaElement().addEventListener(EventTypeNames::webkitfullscreenchange,
                                     this, false);
  media_controls_->GetDocument().addEventListener(
      EventTypeNames::fullscreenchange, this, false);

  // TextTracks events.
  TextTrackList* text_tracks = GetMediaElement().textTracks();
  text_tracks->addEventListener(EventTypeNames::addtrack, this, false);
  text_tracks->addEventListener(EventTypeNames::change, this, false);
  text_tracks->addEventListener(EventTypeNames::removetrack, this, false);

  // Keypress events.
  if (media_controls_->PanelElement()) {
    media_controls_->PanelElement()->addEventListener(EventTypeNames::keypress,
                                                      this, false);
  }

  RemotePlayback* remote = GetRemotePlayback();
  if (remote) {
    remote->addEventListener(EventTypeNames::connect, this);
    remote->addEventListener(EventTypeNames::connecting, this);
    remote->addEventListener(EventTypeNames::disconnect, this);

    // TODO(avayvod, mlamouri): Attach can be called twice. See
    // https://crbug.com/713275.
    if (!remote_playback_availability_callback_id_.has_value()) {
      remote_playback_availability_callback_id_ = base::make_optional(
          remote->WatchAvailabilityInternal(new AvailabilityCallbackWrapper(
              WTF::BindRepeating(&MediaControlsMediaEventListener::
                                     OnRemotePlaybackAvailabilityChanged,
                                 WrapWeakPersistent(this)))));
    }
  }
}

void MediaControlsMediaEventListener::Detach() {
  DCHECK(!GetMediaElement().isConnected());

  media_controls_->GetDocument().removeEventListener(
      EventTypeNames::fullscreenchange, this, false);

  TextTrackList* text_tracks = GetMediaElement().textTracks();
  text_tracks->removeEventListener(EventTypeNames::addtrack, this, false);
  text_tracks->removeEventListener(EventTypeNames::change, this, false);
  text_tracks->removeEventListener(EventTypeNames::removetrack, this, false);

  if (media_controls_->PanelElement()) {
    media_controls_->PanelElement()->removeEventListener(
        EventTypeNames::keypress, this, false);
  }

  RemotePlayback* remote = GetRemotePlayback();
  if (remote) {
    remote->removeEventListener(EventTypeNames::connect, this);
    remote->removeEventListener(EventTypeNames::connecting, this);
    remote->removeEventListener(EventTypeNames::disconnect, this);

    // TODO(avayvod): apparently Detach() can be called without a previous
    // Attach() call. See https://crbug.com/713275 for more details.
    if (remote_playback_availability_callback_id_.has_value() &&
        remote_playback_availability_callback_id_.value() !=
            RemotePlayback::kWatchAvailabilityNotSupported) {
      remote->CancelWatchAvailabilityInternal(
          remote_playback_availability_callback_id_.value());
      remote_playback_availability_callback_id_.reset();
    }
  }
}

bool MediaControlsMediaEventListener::operator==(
    const EventListener& other) const {
  return this == &other;
}

HTMLMediaElement& MediaControlsMediaEventListener::GetMediaElement() {
  return media_controls_->MediaElement();
}

RemotePlayback* MediaControlsMediaEventListener::GetRemotePlayback() {
  return HTMLMediaElementRemotePlayback::remote(GetMediaElement());
}

void MediaControlsMediaEventListener::handleEvent(
    ExecutionContext* execution_context,
    Event* event) {
  if (event->type() == EventTypeNames::volumechange) {
    media_controls_->OnVolumeChange();
    return;
  }
  if (event->type() == EventTypeNames::focusin) {
    media_controls_->OnFocusIn();
    return;
  }
  if (event->type() == EventTypeNames::timeupdate) {
    media_controls_->OnTimeUpdate();
    return;
  }
  if (event->type() == EventTypeNames::durationchange) {
    media_controls_->OnDurationChange();
    return;
  }
  if (event->type() == EventTypeNames::play) {
    media_controls_->OnPlay();
    return;
  }
  if (event->type() == EventTypeNames::playing) {
    media_controls_->OnPlaying();
    return;
  }
  if (event->type() == EventTypeNames::pause) {
    media_controls_->OnPause();
    return;
  }
  if (event->type() == EventTypeNames::error) {
    media_controls_->OnError();
    return;
  }
  if (event->type() == EventTypeNames::loadedmetadata) {
    media_controls_->OnLoadedMetadata();
    return;
  }
  if (event->type() == EventTypeNames::waiting) {
    media_controls_->OnWaiting();
    return;
  }
  if (event->type() == EventTypeNames::progress) {
    media_controls_->OnLoadingProgress();
    return;
  }
  if (event->type() == EventTypeNames::loadeddata) {
    media_controls_->OnLoadedData();
    return;
  }

  // Fullscreen handling.
  if (event->type() == EventTypeNames::fullscreenchange ||
      event->type() == EventTypeNames::webkitfullscreenchange) {
    if (GetMediaElement().IsFullscreen())
      media_controls_->OnEnteredFullscreen();
    else
      media_controls_->OnExitedFullscreen();
    return;
  }

  // TextTracks events.
  if (event->type() == EventTypeNames::addtrack ||
      event->type() == EventTypeNames::removetrack) {
    media_controls_->OnTextTracksAddedOrRemoved();
    return;
  }
  if (event->type() == EventTypeNames::change) {
    media_controls_->OnTextTracksChanged();
    return;
  }

  // Keypress events.
  if (event->type() == EventTypeNames::keypress) {
    if (event->currentTarget() == media_controls_->PanelElement()) {
      media_controls_->OnPanelKeypress();
      return;
    }
  }

  if (event->type() == EventTypeNames::keypress ||
      event->type() == EventTypeNames::keydown ||
      event->type() == EventTypeNames::keyup) {
    media_controls_->OnMediaKeyboardEvent(event);
    return;
  }

  // RemotePlayback state change events.
  if (event->type() == EventTypeNames::connect ||
      event->type() == EventTypeNames::connecting ||
      event->type() == EventTypeNames::disconnect) {
    media_controls_->RemotePlaybackStateChanged();
    return;
  }

  NOTREACHED();
}

void MediaControlsMediaEventListener::OnRemotePlaybackAvailabilityChanged() {
  media_controls_->RefreshCastButtonVisibility();
}

void MediaControlsMediaEventListener::Trace(blink::Visitor* visitor) {
  EventListener::Trace(visitor);
  visitor->Trace(media_controls_);
}

}  // namespace blink
