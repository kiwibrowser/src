// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_PLATFORM_EVENT_DISPATCHER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_PLATFORM_EVENT_DISPATCHER_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/heap/handle.h"

namespace blink {
class PlatformEventController;

class CORE_EXPORT PlatformEventDispatcher : public GarbageCollectedMixin {
 public:
  void AddController(PlatformEventController*);
  void RemoveController(PlatformEventController*);

  void Trace(blink::Visitor*) override;

 protected:
  PlatformEventDispatcher();

  void NotifyControllers();

  virtual void StartListening() = 0;
  virtual void StopListening() = 0;

 private:
  void PurgeControllers();

  HeapHashSet<WeakMember<PlatformEventController>> controllers_;
  bool is_dispatching_;
  bool is_listening_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_PLATFORM_EVENT_DISPATCHER_H_
