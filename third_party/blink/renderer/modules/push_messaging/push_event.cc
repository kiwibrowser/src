// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/push_messaging/push_event.h"

#include "third_party/blink/renderer/modules/push_messaging/push_event_init.h"

namespace blink {

PushEvent::PushEvent(const AtomicString& type,
                     PushMessageData* data,
                     WaitUntilObserver* observer)
    : ExtendableEvent(type, ExtendableEventInit(), observer), data_(data) {}

PushEvent::PushEvent(const AtomicString& type, const PushEventInit& initializer)
    : ExtendableEvent(type, initializer) {
  if (initializer.hasData())
    data_ = PushMessageData::Create(initializer.data());
}

PushEvent::~PushEvent() = default;

const AtomicString& PushEvent::InterfaceName() const {
  return EventNames::PushEvent;
}

PushMessageData* PushEvent::data() {
  return data_.Get();
}

void PushEvent::Trace(blink::Visitor* visitor) {
  visitor->Trace(data_);
  ExtendableEvent::Trace(visitor);
}

}  // namespace blink
