// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/serviceworkers/testing/internals_service_worker.h"

#include "third_party/blink/renderer/modules/serviceworkers/service_worker.h"

namespace blink {

ScriptPromise InternalsServiceWorker::terminateServiceWorker(
    ScriptState* script_state,
    Internals& internals,
    ServiceWorker* worker) {
  return worker->InternalsTerminate(script_state);
}

}  // namespace blink
