// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_HTML_MEDIA_MEDIA_CUSTOM_CONTROLS_FULLSCREEN_DETECTOR_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_HTML_MEDIA_MEDIA_CUSTOM_CONTROLS_FULLSCREEN_DETECTOR_H_

#include "base/macros.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/dom/events/event_listener.h"
#include "third_party/blink/renderer/platform/timer.h"

namespace blink {

class HTMLVideoElement;
class IntRect;
class TimerBase;

class CORE_EXPORT MediaCustomControlsFullscreenDetector final
    : public EventListener {
 public:
  explicit MediaCustomControlsFullscreenDetector(HTMLVideoElement&);

  // EventListener implementation.
  bool operator==(const EventListener&) const override;

  void Attach();
  void Detach();
  void ContextDestroyed();

  void Trace(blink::Visitor*) override;

 private:
  friend class MediaCustomControlsFullscreenDetectorTest;
  friend class HTMLMediaElementEventListenersTest;

  // EventListener implementation.
  void handleEvent(ExecutionContext*, Event*) override;

  HTMLVideoElement& VideoElement() { return *video_element_; }

  void OnCheckViewportIntersectionTimerFired(TimerBase*);

  bool IsVideoOrParentFullscreen();

  static bool ComputeIsDominantVideoForTests(const IntRect& target_rect,
                                             const IntRect& root_rect,
                                             const IntRect& intersection_rect);

  // `m_videoElement` owns |this|.
  Member<HTMLVideoElement> video_element_;
  TaskRunnerTimer<MediaCustomControlsFullscreenDetector>
      check_viewport_intersection_timer_;

  DISALLOW_COPY_AND_ASSIGN(MediaCustomControlsFullscreenDetector);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_HTML_MEDIA_MEDIA_CUSTOM_CONTROLS_FULLSCREEN_DETECTOR_H_
