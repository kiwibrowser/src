// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/presentation/presentation_connection_available_event.h"

#include "third_party/blink/renderer/modules/presentation/presentation_connection_available_event_init.h"

namespace blink {

PresentationConnectionAvailableEvent::~PresentationConnectionAvailableEvent() =
    default;

PresentationConnectionAvailableEvent::PresentationConnectionAvailableEvent(
    const AtomicString& event_type,
    PresentationConnection* connection)
    : Event(event_type, Bubbles::kNo, Cancelable::kNo),
      connection_(connection) {}

PresentationConnectionAvailableEvent::PresentationConnectionAvailableEvent(
    const AtomicString& event_type,
    const PresentationConnectionAvailableEventInit& initializer)
    : Event(event_type, initializer), connection_(initializer.connection()) {}

const AtomicString& PresentationConnectionAvailableEvent::InterfaceName()
    const {
  return EventNames::PresentationConnectionAvailableEvent;
}

void PresentationConnectionAvailableEvent::Trace(blink::Visitor* visitor) {
  visitor->Trace(connection_);
  Event::Trace(visitor);
}

}  // namespace blink
