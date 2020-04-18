// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/event_matcher.h"

#include "ui/events/mojo/event_struct_traits.h"

namespace ui {
namespace ws {

EventMatcher::EventMatcher(const mojom::EventMatcher& matcher)
    : fields_to_match_(NONE),
      event_type_(ui::ET_UNKNOWN),
      event_flags_(ui::EF_NONE),
      ignore_event_flags_(ui::EF_NONE),
      keyboard_code_(ui::VKEY_UNKNOWN),
      pointer_type_(ui::EventPointerType::POINTER_TYPE_UNKNOWN) {
  if (matcher.type_matcher) {
    fields_to_match_ |= TYPE;
    event_type_ = mojo::ConvertTo<ui::EventType>(matcher.type_matcher->type);
  }
  if (matcher.flags_matcher) {
    fields_to_match_ |= FLAGS;
    event_flags_ = matcher.flags_matcher->flags;
    if (matcher.ignore_flags_matcher)
      ignore_event_flags_ = matcher.ignore_flags_matcher->flags;
  }
  if (matcher.key_matcher) {
    fields_to_match_ |= KEYBOARD_CODE;
    keyboard_code_ = static_cast<uint16_t>(matcher.key_matcher->keyboard_code);
  }
  if (matcher.pointer_kind_matcher) {
    fields_to_match_ |= POINTER_KIND;
    switch (matcher.pointer_kind_matcher->pointer_kind) {
      case ui::mojom::PointerKind::MOUSE:
        pointer_type_ = ui::EventPointerType::POINTER_TYPE_MOUSE;
        break;
      case ui::mojom::PointerKind::TOUCH:
        pointer_type_ = ui::EventPointerType::POINTER_TYPE_TOUCH;
        break;
      default:
        NOTREACHED();
    }
  }
  if (matcher.pointer_location_matcher) {
    fields_to_match_ |= POINTER_LOCATION;
    pointer_region_ = matcher.pointer_location_matcher->region;
  }
}

EventMatcher::EventMatcher(EventMatcher&& rhs) = default;

EventMatcher::~EventMatcher() {}

bool EventMatcher::HasFields(int fields) {
  return (fields_to_match_ & fields) != 0;
}

bool EventMatcher::MatchesEvent(const ui::Event& event) const {
  if ((fields_to_match_ & TYPE) && event.type() != event_type_)
    return false;
  // Synthetic flags should never be matched against.
  constexpr int kSyntheticFlags = EF_IS_SYNTHESIZED | EF_IS_REPEAT;
  int flags = event.flags() & ~(ignore_event_flags_ | kSyntheticFlags);
  if ((fields_to_match_ & FLAGS) && flags != event_flags_)
    return false;
  if (fields_to_match_ & KEYBOARD_CODE) {
    if (!event.IsKeyEvent())
      return false;
    if (keyboard_code_ != event.AsKeyEvent()->GetConflatedWindowsKeyCode())
      return false;
  }
  if (fields_to_match_ & POINTER_KIND) {
    if (!event.IsPointerEvent() ||
        pointer_type_ != event.AsPointerEvent()->pointer_details().pointer_type)
      return false;
  }
  if (fields_to_match_ & POINTER_LOCATION) {
    // TODO(sad): The tricky part here is to make sure the same coord-space is
    // used for the location-region and the event-location.
    NOTIMPLEMENTED();
    return false;
  }
  return true;
}

bool EventMatcher::Equals(const EventMatcher& other) const {
  return fields_to_match_ == other.fields_to_match_ &&
         event_type_ == other.event_type_ &&
         event_flags_ == other.event_flags_ &&
         ignore_event_flags_ == other.ignore_event_flags_ &&
         keyboard_code_ == other.keyboard_code_ &&
         pointer_type_ == other.pointer_type_ &&
         pointer_region_ == other.pointer_region_;
}

}  // namespace ws
}  // namespace ui
