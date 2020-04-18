// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_VR_VR_DISPLAY_EVENT_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_VR_VR_DISPLAY_EVENT_H_

#include "third_party/blink/renderer/modules/event_modules.h"
#include "third_party/blink/renderer/modules/vr/vr_display.h"
#include "third_party/blink/renderer/modules/vr/vr_display_event_init.h"

namespace blink {

class VRDisplayEvent final : public Event {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static VRDisplayEvent* Create() { return new VRDisplayEvent; }
  static VRDisplayEvent* Create(const AtomicString& type,
                                VRDisplay* display,
                                String reason) {
    return new VRDisplayEvent(type, display, reason);
  }
  static VRDisplayEvent* Create(const AtomicString& type,
                                VRDisplay*,
                                device::mojom::blink::VRDisplayEventReason);

  static VRDisplayEvent* Create(const AtomicString& type,
                                const VRDisplayEventInit& initializer) {
    return new VRDisplayEvent(type, initializer);
  }

  ~VRDisplayEvent() override;

  VRDisplay* display() const { return display_.Get(); }
  const String& reason() const { return reason_; }

  const AtomicString& InterfaceName() const override;

  void Trace(blink::Visitor*) override;

 private:
  VRDisplayEvent();
  VRDisplayEvent(const AtomicString& type,
                 VRDisplay*,
                 String);
  VRDisplayEvent(const AtomicString&, const VRDisplayEventInit&);

  Member<VRDisplay> display_;
  String reason_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_VR_VR_DISPLAY_EVENT_H_
