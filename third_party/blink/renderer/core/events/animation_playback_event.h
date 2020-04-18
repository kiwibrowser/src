// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_EVENTS_ANIMATION_PLAYBACK_EVENT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_EVENTS_ANIMATION_PLAYBACK_EVENT_H_

#include "base/optional.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/events/animation_playback_event_init.h"

namespace blink {

class AnimationPlaybackEvent final : public Event {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static AnimationPlaybackEvent* Create(const AtomicString& type,
                                        double current_time,
                                        double timeline_time) {
    return new AnimationPlaybackEvent(type, current_time, timeline_time);
  }
  static AnimationPlaybackEvent* Create(
      const AtomicString& type,
      const AnimationPlaybackEventInit& initializer) {
    return new AnimationPlaybackEvent(type, initializer);
  }

  ~AnimationPlaybackEvent() override;

  double currentTime(bool& is_null) const;
  double timelineTime(bool& is_null) const;

  const AtomicString& InterfaceName() const override;

  void Trace(blink::Visitor*) override;

 private:
  AnimationPlaybackEvent(const AtomicString& type,
                         double current_time,
                         double timeline_time);
  AnimationPlaybackEvent(const AtomicString&,
                         const AnimationPlaybackEventInit&);

  base::Optional<double> current_time_;
  base::Optional<double> timeline_time_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_EVENTS_ANIMATION_PLAYBACK_EVENT_H_
