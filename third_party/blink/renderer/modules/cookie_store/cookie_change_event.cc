// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/cookie_store/cookie_change_event.h"

#include "third_party/blink/renderer/modules/cookie_store/cookie_change_event_init.h"
#include "third_party/blink/renderer/modules/cookie_store/cookie_list_item.h"
#include "third_party/blink/renderer/modules/event_modules.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

CookieChangeEvent::~CookieChangeEvent() = default;

const AtomicString& CookieChangeEvent::InterfaceName() const {
  return EventNames::CookieChangeEvent;
}

void CookieChangeEvent::Trace(blink::Visitor* visitor) {
  Event::Trace(visitor);
  visitor->Trace(changed_);
  visitor->Trace(deleted_);
}

CookieChangeEvent::CookieChangeEvent() = default;

CookieChangeEvent::CookieChangeEvent(const AtomicString& type,
                                     HeapVector<CookieListItem> changed,
                                     HeapVector<CookieListItem> deleted)
    : Event(type, Bubbles::kNo, Cancelable::kNo),
      changed_(std::move(changed)),
      deleted_(std::move(deleted)) {}

CookieChangeEvent::CookieChangeEvent(const AtomicString& type,
                                     const CookieChangeEventInit& initializer)
    : Event(type, initializer) {
  if (initializer.hasChanged())
    changed_ = initializer.changed();
  if (initializer.hasDeleted())
    deleted_ = initializer.deleted();
}

}  // namespace blink
