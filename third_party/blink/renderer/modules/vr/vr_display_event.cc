// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/vr/vr_display_event.h"

namespace blink {

namespace {

String VRDisplayEventReasonToString(
    device::mojom::blink::VRDisplayEventReason reason) {
  switch (reason) {
    case device::mojom::blink::VRDisplayEventReason::NONE:
      return "";
    case device::mojom::blink::VRDisplayEventReason::NAVIGATION:
      return "navigation";
    case device::mojom::blink::VRDisplayEventReason::MOUNTED:
      return "mounted";
    case device::mojom::blink::VRDisplayEventReason::UNMOUNTED:
      return "unmounted";
  }

  NOTREACHED();
  return "";
}

}  // namespace

VRDisplayEvent* VRDisplayEvent::Create(
    const AtomicString& type,
    VRDisplay* display,
    device::mojom::blink::VRDisplayEventReason reason) {
  return new VRDisplayEvent(type, display,
                            VRDisplayEventReasonToString(reason));
}

VRDisplayEvent::VRDisplayEvent() = default;

VRDisplayEvent::VRDisplayEvent(const AtomicString& type,
                               VRDisplay* display,
                               String reason)
    : Event(type, Bubbles::kYes, Cancelable::kNo),
      display_(display),
      reason_(reason) {}

VRDisplayEvent::VRDisplayEvent(const AtomicString& type,
                               const VRDisplayEventInit& initializer)
    : Event(type, initializer) {
  if (initializer.hasDisplay())
    display_ = initializer.display();

  if (initializer.hasReason())
    reason_ = initializer.reason();
}

VRDisplayEvent::~VRDisplayEvent() = default;

const AtomicString& VRDisplayEvent::InterfaceName() const {
  return EventNames::VRDisplayEvent;
}

void VRDisplayEvent::Trace(blink::Visitor* visitor) {
  visitor->Trace(display_);
  Event::Trace(visitor);
}

}  // namespace blink
