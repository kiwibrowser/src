// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/html/media/media_custom_controls_fullscreen_detector.h"

#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/public/platform/web_fullscreen_video_status.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/fullscreen/fullscreen.h"
#include "third_party/blink/renderer/core/html/media/html_video_element.h"
#include "third_party/blink/renderer/core/layout/intersection_geometry.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"

namespace blink {

namespace {

constexpr double kCheckFullscreenIntervalSeconds = 1.0f;
constexpr float kMostlyFillViewportThresholdOfOccupationProportion = 0.85f;
constexpr float kMostlyFillViewportThresholdOfVisibleProportion = 0.75f;

}  // anonymous namespace

MediaCustomControlsFullscreenDetector::MediaCustomControlsFullscreenDetector(
    HTMLVideoElement& video)
    : EventListener(kCPPEventListenerType),
      video_element_(video),
      check_viewport_intersection_timer_(
          video.GetDocument().GetTaskRunner(TaskType::kInternalMedia),
          this,
          &MediaCustomControlsFullscreenDetector::
              OnCheckViewportIntersectionTimerFired) {
  if (VideoElement().isConnected())
    Attach();
}

bool MediaCustomControlsFullscreenDetector::operator==(
    const EventListener& other) const {
  return this == &other;
}

void MediaCustomControlsFullscreenDetector::Attach() {
  VideoElement().addEventListener(EventTypeNames::loadedmetadata, this, true);
  VideoElement().GetDocument().addEventListener(
      EventTypeNames::webkitfullscreenchange, this, true);
  VideoElement().GetDocument().addEventListener(
      EventTypeNames::fullscreenchange, this, true);
}

void MediaCustomControlsFullscreenDetector::Detach() {
  VideoElement().removeEventListener(EventTypeNames::loadedmetadata, this,
                                     true);
  VideoElement().GetDocument().removeEventListener(
      EventTypeNames::webkitfullscreenchange, this, true);
  VideoElement().GetDocument().removeEventListener(
      EventTypeNames::fullscreenchange, this, true);
  check_viewport_intersection_timer_.Stop();

  if (VideoElement().GetWebMediaPlayer()) {
    VideoElement().GetWebMediaPlayer()->SetIsEffectivelyFullscreen(
        blink::WebFullscreenVideoStatus::kNotEffectivelyFullscreen);
  }
}

bool MediaCustomControlsFullscreenDetector::ComputeIsDominantVideoForTests(
    const IntRect& target_rect,
    const IntRect& root_rect,
    const IntRect& intersection_rect) {
  if (target_rect.IsEmpty() || root_rect.IsEmpty())
    return false;

  const float x_occupation_proportion =
      1.0f * intersection_rect.Width() / root_rect.Width();
  const float y_occupation_proportion =
      1.0f * intersection_rect.Height() / root_rect.Height();

  // If the viewport is mostly occupied by the video, return true.
  if (std::min(x_occupation_proportion, y_occupation_proportion) >=
      kMostlyFillViewportThresholdOfOccupationProportion) {
    return true;
  }

  // If neither of the dimensions of the viewport is mostly occupied by the
  // video, return false.
  if (std::max(x_occupation_proportion, y_occupation_proportion) <
      kMostlyFillViewportThresholdOfOccupationProportion) {
    return false;
  }

  // If the video is mostly visible in the indominant dimension, return true.
  // Otherwise return false.
  if (x_occupation_proportion > y_occupation_proportion) {
    return target_rect.Height() *
               kMostlyFillViewportThresholdOfVisibleProportion <
           intersection_rect.Height();
  }
  return target_rect.Width() * kMostlyFillViewportThresholdOfVisibleProportion <
         intersection_rect.Width();
}

void MediaCustomControlsFullscreenDetector::handleEvent(
    ExecutionContext* context,
    Event* event) {
  DCHECK(event->type() == EventTypeNames::loadedmetadata ||
         event->type() == EventTypeNames::webkitfullscreenchange ||
         event->type() == EventTypeNames::fullscreenchange);

  // Video is not loaded yet.
  if (VideoElement().getReadyState() < HTMLMediaElement::kHaveMetadata)
    return;

  if (!VideoElement().isConnected() || !IsVideoOrParentFullscreen()) {
    check_viewport_intersection_timer_.Stop();

    if (VideoElement().GetWebMediaPlayer()) {
      VideoElement().GetWebMediaPlayer()->SetIsEffectivelyFullscreen(
          blink::WebFullscreenVideoStatus::kNotEffectivelyFullscreen);
    }

    return;
  }

  check_viewport_intersection_timer_.StartOneShot(
      kCheckFullscreenIntervalSeconds, FROM_HERE);
}

void MediaCustomControlsFullscreenDetector::ContextDestroyed() {
  // This method is called by HTMLVideoElement when it observes context destroy.
  // The reason is that when HTMLMediaElement observes context destroy, it will
  // destroy webMediaPlayer() thus the final
  // setIsEffectivelyFullscreen(kNotEffectivelyFullscreen) is not called.
  Detach();
}

void MediaCustomControlsFullscreenDetector::
    OnCheckViewportIntersectionTimerFired(TimerBase*) {
  DCHECK(IsVideoOrParentFullscreen());
  IntersectionGeometry geometry(nullptr, VideoElement(), Vector<Length>(),
                                true);
  geometry.ComputeGeometry();

  bool is_dominant = ComputeIsDominantVideoForTests(
      geometry.TargetIntRect(), geometry.RootIntRect(),
      geometry.IntersectionIntRect());

  if (!VideoElement().GetWebMediaPlayer())
    return;

  if (!is_dominant) {
    VideoElement().GetWebMediaPlayer()->SetIsEffectivelyFullscreen(
        blink::WebFullscreenVideoStatus::kNotEffectivelyFullscreen);
    return;
  }

  // Picture-in-Picture can be disabled by the website when the API is enabled.
  bool picture_in_picture_allowed =
      !RuntimeEnabledFeatures::PictureInPictureEnabled() &&
      !VideoElement().FastHasAttribute(HTMLNames::disablepictureinpictureAttr);
  if (picture_in_picture_allowed) {
    VideoElement().GetWebMediaPlayer()->SetIsEffectivelyFullscreen(
        blink::WebFullscreenVideoStatus::kFullscreenAndPictureInPictureEnabled);
  } else {
    VideoElement().GetWebMediaPlayer()->SetIsEffectivelyFullscreen(
        blink::WebFullscreenVideoStatus::
            kFullscreenAndPictureInPictureDisabled);
  }
}

bool MediaCustomControlsFullscreenDetector::IsVideoOrParentFullscreen() {
  Element* fullscreen_element =
      Fullscreen::FullscreenElementFrom(VideoElement().GetDocument());
  if (!fullscreen_element)
    return false;

  return fullscreen_element->contains(&VideoElement());
}

void MediaCustomControlsFullscreenDetector::Trace(blink::Visitor* visitor) {
  EventListener::Trace(visitor);
  visitor->Trace(video_element_);
}

}  // namespace blink
