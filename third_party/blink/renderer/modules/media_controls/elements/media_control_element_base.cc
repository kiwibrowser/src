// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/media_controls/elements/media_control_element_base.h"

#include "third_party/blink/renderer/core/html/media/html_media_element.h"
#include "third_party/blink/renderer/modules/media_controls/media_controls_impl.h"

namespace blink {

void MediaControlElementBase::SetIsWanted(bool wanted) {
  if (is_wanted_ == wanted)
    return;

  is_wanted_ = wanted;
  UpdateShownState();
}

bool MediaControlElementBase::IsWanted() const {
  return is_wanted_;
}

void MediaControlElementBase::SetDoesFit(bool fits) {
  does_fit_ = fits;
  UpdateShownState();
}

bool MediaControlElementBase::DoesFit() const {
  return does_fit_;
}

MediaControlElementType MediaControlElementBase::DisplayType() const {
  return display_type_;
}

bool MediaControlElementBase::HasOverflowButton() const {
  return false;
}

MediaControlElementBase::MediaControlElementBase(
    MediaControlsImpl& media_controls,
    MediaControlElementType display_type,
    HTMLElement* element)
    : media_controls_(&media_controls),
      display_type_(display_type),
      element_(element),
      is_wanted_(true),
      does_fit_(true) {}

void MediaControlElementBase::UpdateShownState() {
  if (is_wanted_ && does_fit_)
    element_->RemoveInlineStyleProperty(CSSPropertyDisplay);
  else
    element_->SetInlineStyleProperty(CSSPropertyDisplay, CSSValueNone);
}

MediaControlsImpl& MediaControlElementBase::GetMediaControls() const {
  DCHECK(media_controls_);
  return *media_controls_;
}

HTMLMediaElement& MediaControlElementBase::MediaElement() const {
  return GetMediaControls().MediaElement();
}

void MediaControlElementBase::SetDisplayType(
    MediaControlElementType display_type) {
  display_type_ = display_type;
}

void MediaControlElementBase::Trace(blink::Visitor* visitor) {
  visitor->Trace(media_controls_);
  visitor->Trace(element_);
}

}  // namespace blink
