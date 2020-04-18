// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_CSS_MEDIA_QUERY_LIST_EVENT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_CSS_MEDIA_QUERY_LIST_EVENT_H_

#include "third_party/blink/renderer/core/css/media_query_list.h"
#include "third_party/blink/renderer/core/css/media_query_list_event_init.h"
#include "third_party/blink/renderer/core/dom/events/event.h"

namespace blink {

class MediaQueryListEvent final : public Event {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static MediaQueryListEvent* Create(MediaQueryList* list) {
    return new MediaQueryListEvent(list);
  }

  static MediaQueryListEvent* Create(const String& media, bool matches) {
    return new MediaQueryListEvent(media, matches);
  }

  static MediaQueryListEvent* Create(
      const AtomicString& event_type,
      const MediaQueryListEventInit& initializer) {
    return new MediaQueryListEvent(event_type, initializer);
  }

  String media() const {
    return media_query_list_ ? media_query_list_->media() : media_;
  }
  bool matches() const {
    return media_query_list_ ? media_query_list_->matches() : matches_;
  }

  const AtomicString& InterfaceName() const override {
    return EventNames::MediaQueryListEvent;
  }

  void Trace(blink::Visitor* visitor) override {
    Event::Trace(visitor);
    visitor->Trace(media_query_list_);
  }

 private:
  MediaQueryListEvent(const String& media, bool matches)
      : Event(EventTypeNames::change, Bubbles::kNo, Cancelable::kNo),
        media_(media),
        matches_(matches) {}

  explicit MediaQueryListEvent(MediaQueryList* list)
      : Event(EventTypeNames::change, Bubbles::kNo, Cancelable::kNo),
        media_query_list_(list),
        matches_(false) {}

  MediaQueryListEvent(const AtomicString& event_type,
                      const MediaQueryListEventInit& initializer)
      : Event(event_type, initializer), matches_(false) {
    if (initializer.hasMedia())
      media_ = initializer.media();
    if (initializer.hasMatches())
      matches_ = initializer.matches();
  }

  // We have media_/matches_ for JS-created events; we use media_query_list_
  // for events that blink generates.
  Member<MediaQueryList> media_query_list_;
  String media_;
  bool matches_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_CSS_MEDIA_QUERY_LIST_EVENT_H_
