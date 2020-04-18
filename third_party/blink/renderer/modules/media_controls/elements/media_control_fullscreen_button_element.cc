// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/media_controls/elements/media_control_fullscreen_button_element.h"

#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/html/media/html_media_element.h"
#include "third_party/blink/renderer/core/input_type_names.h"
#include "third_party/blink/renderer/modules/media_controls/media_controls_impl.h"

namespace blink {

MediaControlFullscreenButtonElement::MediaControlFullscreenButtonElement(
    MediaControlsImpl& media_controls)
    : MediaControlInputElement(media_controls, kMediaEnterFullscreenButton) {
  setType(InputTypeNames::button);
  SetShadowPseudoId(AtomicString("-webkit-media-controls-fullscreen-button"));
  SetIsFullscreen(MediaElement().IsFullscreen());
  SetIsWanted(false);
}

void MediaControlFullscreenButtonElement::SetIsFullscreen(bool is_fullscreen) {
  SetDisplayType(is_fullscreen ? kMediaExitFullscreenButton
                               : kMediaEnterFullscreenButton);
  SetClass("fullscreen", is_fullscreen);
}

bool MediaControlFullscreenButtonElement::WillRespondToMouseClickEvents() {
  return true;
}

WebLocalizedString::Name
MediaControlFullscreenButtonElement::GetOverflowStringName() const {
  if (MediaElement().IsFullscreen())
    return WebLocalizedString::kOverflowMenuExitFullscreen;
  return WebLocalizedString::kOverflowMenuEnterFullscreen;
}

bool MediaControlFullscreenButtonElement::HasOverflowButton() const {
  return true;
}

const char* MediaControlFullscreenButtonElement::GetNameForHistograms() const {
  return IsOverflowElement() ? "FullscreenOverflowButton" : "FullscreenButton";
}

void MediaControlFullscreenButtonElement::DefaultEventHandler(Event* event) {
  if (event->type() == EventTypeNames::click) {
    RecordClickMetrics();
    if (MediaElement().IsFullscreen())
      GetMediaControls().ExitFullscreen();
    else
      GetMediaControls().EnterFullscreen();
    event->SetDefaultHandled();
  }
  MediaControlInputElement::DefaultEventHandler(event);
}

void MediaControlFullscreenButtonElement::RecordClickMetrics() {
  bool is_embedded_experience_enabled =
      GetDocument().GetSettings() &&
      GetDocument().GetSettings()->GetEmbeddedMediaExperienceEnabled();

  if (MediaElement().IsFullscreen()) {
    Platform::Current()->RecordAction(
        UserMetricsAction("Media.Controls.ExitFullscreen"));
    if (is_embedded_experience_enabled) {
      Platform::Current()->RecordAction(UserMetricsAction(
          "Media.Controls.ExitFullscreen.EmbeddedExperience"));
    }
  } else {
    Platform::Current()->RecordAction(
        UserMetricsAction("Media.Controls.EnterFullscreen"));
    if (is_embedded_experience_enabled) {
      Platform::Current()->RecordAction(UserMetricsAction(
          "Media.Controls.EnterFullscreen.EmbeddedExperience"));
    }
  }
}

}  // namespace blink
