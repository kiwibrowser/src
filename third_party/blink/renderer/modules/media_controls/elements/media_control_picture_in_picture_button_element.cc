// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/media_controls/elements/media_control_picture_in_picture_button_element.h"

#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/html/media/html_media_element.h"
#include "third_party/blink/renderer/core/html/media/html_media_source.h"
#include "third_party/blink/renderer/core/html/media/html_video_element.h"
#include "third_party/blink/renderer/core/input_type_names.h"
#include "third_party/blink/renderer/modules/media_controls/media_controls_impl.h"
#include "third_party/blink/renderer/modules/picture_in_picture/picture_in_picture_controller_impl.h"

namespace blink {

MediaControlPictureInPictureButtonElement::
    MediaControlPictureInPictureButtonElement(MediaControlsImpl& media_controls)
    : MediaControlInputElement(media_controls, kMediaPlayButton) {
  setType(InputTypeNames::button);
  SetShadowPseudoId(
      AtomicString("-internal-media-controls-picture-in-picture-button"));
  SetIsWanted(false);
}

bool MediaControlPictureInPictureButtonElement::
    WillRespondToMouseClickEvents() {
  return true;
}

WebLocalizedString::Name
MediaControlPictureInPictureButtonElement::GetOverflowStringName() const {
  return WebLocalizedString::kOverflowMenuPictureInPicture;
}

bool MediaControlPictureInPictureButtonElement::HasOverflowButton() const {
  return true;
}

const char* MediaControlPictureInPictureButtonElement::GetNameForHistograms()
    const {
  return IsOverflowElement() ? "PictureInPictureOverflowButton"
                             : "PictureInPictureButton";
}

void MediaControlPictureInPictureButtonElement::DefaultEventHandler(
    Event* event) {
  if (event->type() == EventTypeNames::click) {
    PictureInPictureControllerImpl& controller =
        PictureInPictureControllerImpl::From(MediaElement().GetDocument());

    DCHECK(MediaElement().IsHTMLVideoElement());
    // TODO(crbug.com/840516): Toggle PiP instead.
    controller.EnterPictureInPicture(&ToHTMLVideoElement(MediaElement()),
                                     nullptr);
  }

  MediaControlInputElement::DefaultEventHandler(event);
}

}  // namespace blink
