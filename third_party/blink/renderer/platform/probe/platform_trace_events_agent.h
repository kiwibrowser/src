// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_PROBE_PLATFORM_TRACE_EVENTS_AGENT_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_PROBE_PLATFORM_TRACE_EVENTS_AGENT_H_

#include "third_party/blink/renderer/platform/heap/heap.h"
#include "third_party/blink/renderer/platform/platform_export.h"

namespace blink {

namespace probe {
class PlatformSendRequest;
}

class PLATFORM_EXPORT PlatformTraceEventsAgent
    : public GarbageCollected<PlatformTraceEventsAgent> {
 public:
  void Trace(blink::Visitor* visitor) {}

  void Will(const probe::PlatformSendRequest&);
  void Did(const probe::PlatformSendRequest&);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_PROBE_PLATFORM_TRACE_EVENTS_AGENT_H_
