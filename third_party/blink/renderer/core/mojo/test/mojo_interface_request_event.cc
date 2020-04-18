// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/mojo/test/mojo_interface_request_event.h"

#include "third_party/blink/renderer/core/mojo/mojo_handle.h"
#include "third_party/blink/renderer/core/mojo/test/mojo_interface_request_event_init.h"

namespace blink {

MojoInterfaceRequestEvent::~MojoInterfaceRequestEvent() = default;

void MojoInterfaceRequestEvent::Trace(blink::Visitor* visitor) {
  Event::Trace(visitor);
  visitor->Trace(handle_);
}

MojoInterfaceRequestEvent::MojoInterfaceRequestEvent(MojoHandle* handle)
    : Event(EventTypeNames::interfacerequest, Bubbles::kNo, Cancelable::kNo),
      handle_(handle) {}

MojoInterfaceRequestEvent::MojoInterfaceRequestEvent(
    const AtomicString& type,
    const MojoInterfaceRequestEventInit& initializer)
    : Event(type, Bubbles::kNo, Cancelable::kNo),
      handle_(initializer.handle()) {}

}  // namespace blink
