// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/media_controls/elements/media_control_overlay_play_button_element.h"

#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/public/platform/web_size.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/dom/shadow_root.h"
#include "third_party/blink/renderer/core/events/mouse_event.h"
#include "third_party/blink/renderer/core/geometry/dom_rect.h"
#include "third_party/blink/renderer/core/html/html_style_element.h"
#include "third_party/blink/renderer/core/html/media/html_media_element.h"
#include "third_party/blink/renderer/core/html/media/html_media_source.h"
#include "third_party/blink/renderer/core/input_type_names.h"
#include "third_party/blink/renderer/modules/media_controls/elements/media_control_elements_helper.h"
#include "third_party/blink/renderer/modules/media_controls/media_controls_impl.h"
#include "third_party/blink/renderer/modules/media_controls/media_controls_resource_loader.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"
#include "third_party/blink/renderer/platform/wtf/time.h"

namespace {

// The size of the inner circle button in pixels.
constexpr int kInnerButtonSize = 56;

// The touch padding of the inner circle button in pixels.
constexpr int kInnerButtonTouchPaddingSize = 20;

// Check if a point is based within the boundary of a DOMRect with a margin.
bool IsPointInRect(blink::DOMRect& rect, int margin, int x, int y) {
  return ((x >= (rect.left() - margin)) && (x <= (rect.right() + margin)) &&
          (y >= (rect.top() - margin)) && (y <= (rect.bottom() + margin)));
}

// The delay between two taps to be recognized as a double tap gesture.
constexpr WTF::TimeDelta kDoubleTapDelay = TimeDelta::FromMilliseconds(300);

// The number of seconds to jump when double tapping.
constexpr int kNumberOfSecondsToJump = 10;

// The CSS class to add to hide the element.
const char kHiddenClassName[] = "hidden";

}  // namespace.

namespace blink {

MediaControlOverlayPlayButtonElement::AnimatedArrow::AnimatedArrow(
    const AtomicString& id,
    Document& document)
    : HTMLDivElement(document) {
  setAttribute("id", id);
}

void MediaControlOverlayPlayButtonElement::AnimatedArrow::HideInternal() {
  DCHECK(!hidden_);
  svg_container_->SetInlineStyleProperty(CSSPropertyDisplay, CSSValueNone);
  hidden_ = true;
}

void MediaControlOverlayPlayButtonElement::AnimatedArrow::ShowInternal() {
  DCHECK(hidden_);
  hidden_ = false;

  if (svg_container_) {
    svg_container_->RemoveInlineStyleProperty(CSSPropertyDisplay);
    return;
  }

  SetInnerHTMLFromString(MediaControlsResourceLoader::GetJumpSVGImage());

  last_arrow_ = getElementById("arrow-3");
  svg_container_ = getElementById("jump");

  event_listener_ = new MediaControlAnimationEventListener(this);
}

void MediaControlOverlayPlayButtonElement::AnimatedArrow::
    OnAnimationIteration() {
  counter_--;

  if (counter_ == 0)
    HideInternal();
}

void MediaControlOverlayPlayButtonElement::AnimatedArrow::Show() {
  if (hidden_)
    ShowInternal();

  counter_++;
}

Element&
MediaControlOverlayPlayButtonElement::AnimatedArrow::WatchedAnimationElement()
    const {
  return *last_arrow_;
}

void MediaControlOverlayPlayButtonElement::AnimatedArrow::Trace(
    Visitor* visitor) {
  MediaControlAnimationEventListener::Observer::Trace(visitor);
  HTMLDivElement::Trace(visitor);
  visitor->Trace(last_arrow_);
  visitor->Trace(svg_container_);
  visitor->Trace(event_listener_);
}

// The DOM structure looks like:
//
// MediaControlOverlayPlayButtonElement
//   (-webkit-media-controls-overlay-play-button)
// +-div (-internal-media-controls-overlay-play-button-internal)
//   {if MediaControlsImpl::IsModern}
//   This contains the inner circle with the actual play/pause icon.
MediaControlOverlayPlayButtonElement::MediaControlOverlayPlayButtonElement(
    MediaControlsImpl& media_controls)
    : MediaControlInputElement(media_controls, kMediaPlayButton),
      tap_timer_(GetDocument().GetTaskRunner(TaskType::kMediaElementEvent),
                 this,
                 &MediaControlOverlayPlayButtonElement::TapTimerFired),
      internal_button_(nullptr),
      left_jump_arrow_(nullptr),
      right_jump_arrow_(nullptr) {
  EnsureUserAgentShadowRoot();
  setType(InputTypeNames::button);
  SetShadowPseudoId(AtomicString("-webkit-media-controls-overlay-play-button"));

  if (MediaControlsImpl::IsModern()) {
    internal_button_ = MediaControlElementsHelper::CreateDiv(
        "-internal-media-controls-overlay-play-button-internal",
        GetShadowRoot());
  }
}

void MediaControlOverlayPlayButtonElement::UpdateDisplayType() {
  SetIsWanted(MediaElement().ShouldShowControls() &&
              (MediaControlsImpl::IsModern() || MediaElement().paused()));
  if (MediaControlsImpl::IsModern()) {
    SetDisplayType(MediaElement().paused() ? kMediaPlayButton
                                           : kMediaPauseButton);
  }
  MediaControlInputElement::UpdateDisplayType();
}

const char* MediaControlOverlayPlayButtonElement::GetNameForHistograms() const {
  return "PlayOverlayButton";
}

void MediaControlOverlayPlayButtonElement::MaybePlayPause() {
  if (MediaElement().paused()) {
    Platform::Current()->RecordAction(
        UserMetricsAction("Media.Controls.PlayOverlay"));
  } else {
    Platform::Current()->RecordAction(
        UserMetricsAction("Media.Controls.PauseOverlay"));
  }

  // Allow play attempts for plain src= media to force a reload in the error
  // state. This allows potential recovery for transient network and decoder
  // resource issues.
  const String& url = MediaElement().currentSrc().GetString();
  if (MediaElement().error() && !HTMLMediaElement::IsMediaStreamURL(url) &&
      !HTMLMediaSource::Lookup(url)) {
    MediaElement().load();
  }

  MediaElement().TogglePlayState();

  // If we triggered a play event then we should quickly hide the button.
  if (!MediaElement().paused())
    SetIsDisplayed(false);

  MaybeRecordInteracted();
  UpdateDisplayType();
}

void MediaControlOverlayPlayButtonElement::MaybeJump(int seconds) {
  // Load the arrow icons and associate CSS the first time we jump.
  if (!left_jump_arrow_) {
    DCHECK(!right_jump_arrow_);
    ShadowRoot* shadow_root = GetShadowRoot();

    // This stylesheet element and will contain rules that are specific to the
    // jump arrows. The shadow DOM protects these rules from the parent DOM
    // from bleeding across the shadow DOM boundary.
    auto* style = HTMLStyleElement::Create(GetDocument(), CreateElementFlags());
    style->setTextContent(
        MediaControlsResourceLoader::GetOverlayPlayStyleSheet());
    shadow_root->ParserAppendChild(style);

    // Insert the left jump arrow to the left of the play button.
    left_jump_arrow_ = new MediaControlOverlayPlayButtonElement::AnimatedArrow(
        "left-arrow", GetDocument());
    shadow_root->ParserInsertBefore(left_jump_arrow_,
                                    *shadow_root->firstChild());

    // Insert the right jump arrow to the right of the play button.
    right_jump_arrow_ = new MediaControlOverlayPlayButtonElement::AnimatedArrow(
        "right-arrow", GetDocument());
    shadow_root->ParserAppendChild(right_jump_arrow_);
  }

  DCHECK(left_jump_arrow_ && right_jump_arrow_);
  double new_time = std::max(0.0, MediaElement().currentTime() + seconds);
  new_time = std::min(new_time, MediaElement().duration());
  MediaElement().setCurrentTime(new_time);

  if (seconds > 0)
    right_jump_arrow_->Show();
  else
    left_jump_arrow_->Show();
}

void MediaControlOverlayPlayButtonElement::DefaultEventHandler(Event* event) {
  if (ShouldCausePlayPause(event)) {
    event->SetDefaultHandled();
    MaybePlayPause();
  } else if (event->type() == EventTypeNames::click) {
    event->SetDefaultHandled();

    DCHECK(event->IsMouseEvent());
    MouseEvent* mouse_event = ToMouseEvent(event);
    DCHECK(mouse_event->HasPosition());

    if (!tap_timer_.IsActive()) {
      // If there was not a previous touch and this was outside of the button
      // then we should toggle visibility with a small unnoticeable delay in
      // case their is a second tap.
      if (tap_timer_.IsActive())
        return;
      tap_was_touch_event_ = MediaControlsImpl::IsTouchEvent(event);
      tap_timer_.StartOneShot(kDoubleTapDelay, FROM_HERE);
    } else {
      // Cancel the play pause event.
      tap_timer_.Stop();

      // If both taps were touch events, then jump.
      if (tap_was_touch_event_.value() &&
          MediaControlsImpl::IsTouchEvent(event)) {
        // Jump forwards or backwards based on the position of the tap.
        WebSize element_size =
            MediaControlElementsHelper::GetSizeOrDefault(*this, WebSize(0, 0));

        if (mouse_event->clientX() >= element_size.width / 2) {
          MaybeJump(kNumberOfSecondsToJump);
        } else {
          MaybeJump(kNumberOfSecondsToJump * -1);
        }
      } else {
        if (GetMediaControls().IsFullscreenEnabled()) {
          // Enter or exit fullscreen.
          if (MediaElement().IsFullscreen())
            GetMediaControls().ExitFullscreen();
          else
            GetMediaControls().EnterFullscreen();
        }
      }

      tap_was_touch_event_.reset();
    }
  }
  MediaControlInputElement::DefaultEventHandler(event);
}

bool MediaControlOverlayPlayButtonElement::KeepEventInNode(Event* event) {
  return ShouldCausePlayPause(event);
}

bool MediaControlOverlayPlayButtonElement::ShouldCausePlayPause(
    Event* event) const {
  // Only click events cause a play/pause.
  if (event->type() != EventTypeNames::click)
    return false;

  // Double tap to navigate should only be available on modern controls.
  if (!MediaControlsImpl::IsModern() || !event->IsMouseEvent())
    return true;

  // If the event doesn't have position data we should just default to
  // play/pause.
  // TODO(beccahughes): Move to PointerEvent.
  MouseEvent* mouse_event = ToMouseEvent(event);
  if (!mouse_event->HasPosition())
    return true;

  // If the click happened on the internal button or a margin around it then
  // we should play/pause.
  return IsPointInRect(*internal_button_->getBoundingClientRect(),
                       kInnerButtonTouchPaddingSize, mouse_event->clientX(),
                       mouse_event->clientY());
}

WebSize MediaControlOverlayPlayButtonElement::GetSizeOrDefault() const {
  // The size should come from the internal button which actually displays the
  // button.
  return MediaControlElementsHelper::GetSizeOrDefault(
      *internal_button_, WebSize(kInnerButtonSize, kInnerButtonSize));
}

void MediaControlOverlayPlayButtonElement::SetIsDisplayed(bool displayed) {
  if (displayed == displayed_)
    return;

  SetClass(kHiddenClassName, !displayed);
  displayed_ = displayed;
}

void MediaControlOverlayPlayButtonElement::TapTimerFired(TimerBase*) {
  if (tap_was_touch_event_.value())
    GetMediaControls().MaybeToggleControlsFromTap();
  tap_was_touch_event_.reset();
}

void MediaControlOverlayPlayButtonElement::Trace(blink::Visitor* visitor) {
  MediaControlInputElement::Trace(visitor);
  visitor->Trace(internal_button_);
  visitor->Trace(left_jump_arrow_);
  visitor->Trace(right_jump_arrow_);
}

}  // namespace blink
