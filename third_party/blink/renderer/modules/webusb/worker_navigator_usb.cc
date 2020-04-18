// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/webusb/worker_navigator_usb.h"

#include "third_party/blink/renderer/core/workers/worker_navigator.h"
#include "third_party/blink/renderer/modules/webusb/usb.h"

namespace blink {

WorkerNavigatorUSB& WorkerNavigatorUSB::From(
    WorkerNavigator& worker_navigator) {
  WorkerNavigatorUSB* supplement =
      Supplement<WorkerNavigator>::From<WorkerNavigatorUSB>(worker_navigator);
  if (!supplement) {
    supplement = new WorkerNavigatorUSB(worker_navigator);
    ProvideTo(worker_navigator, supplement);
  }
  return *supplement;
}

USB* WorkerNavigatorUSB::usb(ScriptState* script_state,
                             WorkerNavigator& worker_navigator) {
  return WorkerNavigatorUSB::From(worker_navigator).usb(script_state);
}

USB* WorkerNavigatorUSB::usb(ScriptState* script_state) {
  // A bug in the WebIDL compiler causes this attribute to be exposed to the
  // WorkerNavigator interface in the ServiceWorkerGlobalScope, therefore we
  // will just return the empty usb_ member if the current context is a
  // ServiceWorkerGlobalScope.
  // TODO(https://crbug.com/839117): Once this attribute stops being exposed to
  // the WorkerNavigator for a Service worker, remove this check.
  if (!usb_ &&
      !ExecutionContext::From(script_state)->IsServiceWorkerGlobalScope()) {
    DCHECK(ExecutionContext::From(script_state));
    usb_ = USB::Create(*ExecutionContext::From(script_state));
  }
  return usb_;
}

void WorkerNavigatorUSB::Trace(blink::Visitor* visitor) {
  visitor->Trace(usb_);
  Supplement<WorkerNavigator>::Trace(visitor);
}

WorkerNavigatorUSB::WorkerNavigatorUSB(WorkerNavigator& worker_navigator)
    : Supplement<WorkerNavigator>(worker_navigator) {}

const char WorkerNavigatorUSB::kSupplementName[] = "WorkerNavigatorUSB";

}  // namespace blink
