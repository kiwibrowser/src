// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/dom/document_shutdown_observer.h"

#include "third_party/blink/renderer/core/dom/document_shutdown_notifier.h"

namespace blink {

void DocumentShutdownObserver::ContextDestroyed(Document*) {}

DocumentShutdownObserver::DocumentShutdownObserver()
    : LifecycleObserver(nullptr) {}

}  // namespace blink
