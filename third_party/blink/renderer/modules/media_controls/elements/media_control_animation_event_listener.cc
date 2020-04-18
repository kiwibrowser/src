// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/media_controls/elements/media_control_animation_event_listener.h"

#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/core/dom/events/event.h"

namespace blink {

MediaControlAnimationEventListener::MediaControlAnimationEventListener(
    Observer* observer)
    : EventListener(EventListener::kCPPEventListenerType), observer_(observer) {
  observer_->WatchedAnimationElement().addEventListener(
      EventTypeNames::animationend, this, false);
  observer_->WatchedAnimationElement().addEventListener(
      EventTypeNames::animationiteration, this, false);
}

void MediaControlAnimationEventListener::Detach() {
  observer_->WatchedAnimationElement().removeEventListener(
      EventTypeNames::animationend, this, false);
  observer_->WatchedAnimationElement().removeEventListener(
      EventTypeNames::animationiteration, this, false);
}

bool MediaControlAnimationEventListener::operator==(
    const EventListener& other) const {
  return this == &other;
}

void MediaControlAnimationEventListener::Trace(Visitor* visitor) {
  visitor->Trace(observer_);
  EventListener::Trace(visitor);
}

void MediaControlAnimationEventListener::handleEvent(ExecutionContext* context,
                                                     Event* event) {
  if (event->type() == EventTypeNames::animationend) {
    observer_->OnAnimationEnd();
    return;
  }
  if (event->type() == EventTypeNames::animationiteration) {
    observer_->OnAnimationIteration();
    return;
  }

  NOTREACHED();
}

void MediaControlAnimationEventListener::Observer::Trace(Visitor*) {}

}  // namespace blink
