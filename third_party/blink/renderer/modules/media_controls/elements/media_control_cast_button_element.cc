// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/media_controls/elements/media_control_cast_button_element.h"

#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/geometry/dom_rect.h"
#include "third_party/blink/renderer/core/html/media/html_media_element.h"
#include "third_party/blink/renderer/core/input_type_names.h"
#include "third_party/blink/renderer/modules/media_controls/elements/media_control_elements_helper.h"
#include "third_party/blink/renderer/modules/media_controls/media_controls_impl.h"
#include "third_party/blink/renderer/modules/remoteplayback/html_media_element_remote_playback.h"
#include "third_party/blink/renderer/modules/remoteplayback/remote_playback.h"

namespace blink {

namespace {

Element* ElementFromCenter(Element& element) {
  DOMRect* client_rect = element.getBoundingClientRect();
  int center_x =
      static_cast<int>((client_rect->left() + client_rect->right()) / 2);
  int center_y =
      static_cast<int>((client_rect->top() + client_rect->bottom()) / 2);

  return element.GetDocument().ElementFromPoint(center_x, center_y);
}

}  // anonymous namespace

MediaControlCastButtonElement::MediaControlCastButtonElement(
    MediaControlsImpl& media_controls,
    bool is_overlay_button)
    : MediaControlInputElement(media_controls, kMediaCastOnButton),
      is_overlay_button_(is_overlay_button) {
  SetShadowPseudoId(is_overlay_button
                        ? "-internal-media-controls-overlay-cast-button"
                        : "-internal-media-controls-cast-button");
  setType(InputTypeNames::button);
  UpdateDisplayType();
}

void MediaControlCastButtonElement::TryShowOverlay() {
  DCHECK(is_overlay_button_);

  SetIsWanted(true);
  if (ElementFromCenter(*this) != &MediaElement()) {
    SetIsWanted(false);
    return;
  }
}

void MediaControlCastButtonElement::UpdateDisplayType() {
  if (IsPlayingRemotely()) {
    if (is_overlay_button_) {
      SetDisplayType(kMediaOverlayCastOnButton);
    } else {
      SetDisplayType(kMediaCastOnButton);
    }
  } else {
    if (is_overlay_button_) {
      SetDisplayType(kMediaOverlayCastOffButton);
    } else {
      SetDisplayType(kMediaCastOffButton);
    }
  }
  UpdateOverflowString();
  SetClass("on", IsPlayingRemotely());

  MediaControlInputElement::UpdateDisplayType();
}

bool MediaControlCastButtonElement::WillRespondToMouseClickEvents() {
  return true;
}

WebLocalizedString::Name MediaControlCastButtonElement::GetOverflowStringName()
    const {
  return WebLocalizedString::kOverflowMenuCast;
}

bool MediaControlCastButtonElement::HasOverflowButton() const {
  return true;
}

const char* MediaControlCastButtonElement::GetNameForHistograms() const {
  return is_overlay_button_
             ? "CastOverlayButton"
             : IsOverflowElement() ? "CastOverflowButton" : "CastButton";
}

void MediaControlCastButtonElement::DefaultEventHandler(Event* event) {
  if (event->type() == EventTypeNames::click) {
    if (is_overlay_button_) {
      Platform::Current()->RecordAction(
          UserMetricsAction("Media.Controls.CastOverlay"));
      Platform::Current()->RecordRapporURL("Media.Controls.CastOverlay",
                                           WebURL(GetDocument().Url()));
    } else {
      Platform::Current()->RecordAction(
          UserMetricsAction("Media.Controls.Cast"));
      Platform::Current()->RecordRapporURL("Media.Controls.Cast",
                                           WebURL(GetDocument().Url()));
    }

    RemotePlayback* remote =
        HTMLMediaElementRemotePlayback::remote(MediaElement());
    if (remote)
      remote->PromptInternal();
  }
  MediaControlInputElement::DefaultEventHandler(event);
}

bool MediaControlCastButtonElement::KeepEventInNode(Event* event) {
  return MediaControlElementsHelper::IsUserInteractionEvent(event);
}

bool MediaControlCastButtonElement::IsPlayingRemotely() const {
  RemotePlayback* remote =
      HTMLMediaElementRemotePlayback::remote(MediaElement());
  return remote && remote->GetState() != WebRemotePlaybackState::kDisconnected;
}

}  // namespace blink
