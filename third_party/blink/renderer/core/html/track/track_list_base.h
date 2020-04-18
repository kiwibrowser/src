// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_HTML_TRACK_TRACK_LIST_BASE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_HTML_TRACK_TRACK_LIST_BASE_H_

#include "third_party/blink/renderer/core/dom/events/event_target.h"
#include "third_party/blink/renderer/core/html/media/html_media_element.h"
#include "third_party/blink/renderer/core/html/track/track_event.h"
#include "third_party/blink/renderer/core/html/track/track_event_init.h"
#include "third_party/blink/renderer/platform/bindings/trace_wrapper_member.h"

namespace blink {

template <class T>
class TrackListBase : public EventTargetWithInlineData {
 public:
  explicit TrackListBase(HTMLMediaElement* media_element)
      : media_element_(media_element) {}

  ~TrackListBase() override = default;

  unsigned length() const { return tracks_.size(); }
  T* AnonymousIndexedGetter(unsigned index) const {
    if (index >= tracks_.size())
      return nullptr;
    return tracks_[index].Get();
  }

  T* getTrackById(const String& id) const {
    for (const auto& track : tracks_) {
      if (String(track->id()) == id)
        return track.Get();
    }

    return nullptr;
  }

  DEFINE_ATTRIBUTE_EVENT_LISTENER(change);
  DEFINE_ATTRIBUTE_EVENT_LISTENER(addtrack);
  DEFINE_ATTRIBUTE_EVENT_LISTENER(removetrack);

  // EventTarget interface
  ExecutionContext* GetExecutionContext() const override {
    if (media_element_)
      return media_element_->GetExecutionContext();
    return nullptr;
  }

  void Add(T* track) {
    track->SetMediaElement(media_element_);
    tracks_.push_back(track);
    ScheduleEvent(TrackEvent::Create(EventTypeNames::addtrack, track));
  }

  void Remove(WebMediaPlayer::TrackId track_id) {
    for (unsigned i = 0; i < tracks_.size(); ++i) {
      if (tracks_[i]->id() != track_id)
        continue;

      tracks_[i]->SetMediaElement(nullptr);
      ScheduleEvent(
          TrackEvent::Create(EventTypeNames::removetrack, tracks_[i].Get()));
      tracks_.EraseAt(i);
      return;
    }
    NOTREACHED();
  }

  void RemoveAll() {
    for (const auto& track : tracks_)
      track->SetMediaElement(nullptr);

    tracks_.clear();
  }

  void ScheduleChangeEvent() {
    ScheduleEvent(Event::Create(EventTypeNames::change));
  }

  void Trace(blink::Visitor* visitor) override {
    visitor->Trace(tracks_);
    visitor->Trace(media_element_);
    EventTargetWithInlineData::Trace(visitor);
  }

  void TraceWrappers(ScriptWrappableVisitor* visitor) const override {
    for (auto track : tracks_) {
      visitor->TraceWrappers(track);
    }
    EventTargetWithInlineData::TraceWrappers(visitor);
  }

 private:
  void ScheduleEvent(Event* event) {
    event->SetTarget(this);
    media_element_->ScheduleEvent(event);
  }

  HeapVector<TraceWrapperMember<T>> tracks_;
  Member<HTMLMediaElement> media_element_;
};

}  // namespace blink

#endif
