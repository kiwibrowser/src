// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_WEBUSB_WORKER_NAVIGATOR_USB_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_WEBUSB_WORKER_NAVIGATOR_USB_H_

#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/workers/worker_navigator.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/supplementable.h"

namespace blink {

class USB;
class WorkerNavigator;

class WorkerNavigatorUSB final : public GarbageCollected<WorkerNavigatorUSB>,
                                 public Supplement<WorkerNavigator> {
  USING_GARBAGE_COLLECTED_MIXIN(WorkerNavigatorUSB);

 public:
  static const char kSupplementName[];

  // Gets, or creates, WorkerNavigatorUSB supplement on WorkerNavigator.
  // See platform/Supplementable.h
  static WorkerNavigatorUSB& From(WorkerNavigator&);

  static USB* usb(ScriptState*, WorkerNavigator&);
  USB* usb(ScriptState*);

  void Trace(blink::Visitor*) override;

 private:
  explicit WorkerNavigatorUSB(WorkerNavigator&);

  Member<USB> usb_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_WEBUSB_WORKER_NAVIGATOR_USB_H_
