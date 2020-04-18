// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/cookie_store/extendable_cookie_change_event.h"

#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/renderer/modules/cookie_store/cookie_list_item.h"
#include "third_party/blink/renderer/modules/cookie_store/extendable_cookie_change_event_init.h"
#include "third_party/blink/renderer/modules/event_modules.h"
#include "third_party/blink/renderer/modules/serviceworkers/extendable_event_init.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

ExtendableCookieChangeEvent::~ExtendableCookieChangeEvent() = default;

const AtomicString& ExtendableCookieChangeEvent::InterfaceName() const {
  return EventNames::ExtendableCookieChangeEvent;
}

void ExtendableCookieChangeEvent::Trace(blink::Visitor* visitor) {
  ExtendableEvent::Trace(visitor);
  visitor->Trace(changed_);
  visitor->Trace(deleted_);
}

ExtendableCookieChangeEvent::ExtendableCookieChangeEvent(
    const AtomicString& type,
    HeapVector<CookieListItem> changed,
    HeapVector<CookieListItem> deleted,
    WaitUntilObserver* wait_until_observer)
    : ExtendableEvent(type, ExtendableEventInit(), wait_until_observer),
      changed_(std::move(changed)),
      deleted_(std::move(deleted)) {}

ExtendableCookieChangeEvent::ExtendableCookieChangeEvent(
    const AtomicString& type,
    const ExtendableCookieChangeEventInit& initializer)
    : ExtendableEvent(type, initializer) {
  if (initializer.hasChanged())
    changed_ = initializer.changed();
  if (initializer.hasDeleted())
    deleted_ = initializer.deleted();
}

// static
void ExtendableCookieChangeEvent::ToCookieChangeListItem(
    const WebString& cookie_name,
    const WebString& cookie_value,
    bool is_cookie_delete,
    HeapVector<CookieListItem>& changed,
    HeapVector<CookieListItem>& deleted) {
  if (is_cookie_delete) {
    CookieListItem& cookie = deleted.emplace_back();
    cookie.setName(cookie_name);
  } else {
    CookieListItem& cookie = changed.emplace_back();
    cookie.setName(cookie_name);
    cookie.setValue(cookie_value);
  }
}

}  // namespace blink
