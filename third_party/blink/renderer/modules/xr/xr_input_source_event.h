// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_XR_XR_INPUT_SOURCE_EVENT_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_XR_XR_INPUT_SOURCE_EVENT_H_

#include "third_party/blink/renderer/modules/event_modules.h"
#include "third_party/blink/renderer/modules/xr/xr_input_source.h"
#include "third_party/blink/renderer/modules/xr/xr_input_source_event_init.h"
#include "third_party/blink/renderer/modules/xr/xr_presentation_frame.h"

namespace blink {

class XRInputSourceEvent final : public Event {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static XRInputSourceEvent* Create() { return new XRInputSourceEvent; }
  static XRInputSourceEvent* Create(const AtomicString& type,
                                    XRPresentationFrame* frame,
                                    XRInputSource* input_source) {
    return new XRInputSourceEvent(type, frame, input_source);
  }

  static XRInputSourceEvent* Create(const AtomicString& type,
                                    const XRInputSourceEventInit& initializer) {
    return new XRInputSourceEvent(type, initializer);
  }

  ~XRInputSourceEvent() override;

  XRPresentationFrame* frame() const { return frame_.Get(); }
  XRInputSource* inputSource() const { return input_source_.Get(); }

  const AtomicString& InterfaceName() const override;

  void Trace(blink::Visitor*) override;

 private:
  XRInputSourceEvent();
  XRInputSourceEvent(const AtomicString& type,
                     XRPresentationFrame*,
                     XRInputSource*);
  XRInputSourceEvent(const AtomicString&, const XRInputSourceEventInit&);

  Member<XRPresentationFrame> frame_;
  Member<XRInputSource> input_source_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_XR_XR_INPUT_SOURCE_EVENT_H_
