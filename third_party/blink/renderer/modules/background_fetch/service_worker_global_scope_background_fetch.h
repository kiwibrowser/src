// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_BACKGROUND_FETCH_SERVICE_WORKER_GLOBAL_SCOPE_BACKGROUND_FETCH_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_BACKGROUND_FETCH_SERVICE_WORKER_GLOBAL_SCOPE_BACKGROUND_FETCH_H_

#include "third_party/blink/renderer/core/dom/events/event_target.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"

namespace blink {

class ServiceWorkerGlobalScopeBackgroundFetch {
  STATIC_ONLY(ServiceWorkerGlobalScopeBackgroundFetch);

 public:
  DEFINE_STATIC_ATTRIBUTE_EVENT_LISTENER(backgroundfetched);
  DEFINE_STATIC_ATTRIBUTE_EVENT_LISTENER(backgroundfetchfail);
  DEFINE_STATIC_ATTRIBUTE_EVENT_LISTENER(backgroundfetchabort);
  DEFINE_STATIC_ATTRIBUTE_EVENT_LISTENER(backgroundfetchclick);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_BACKGROUND_FETCH_SERVICE_WORKER_GLOBAL_SCOPE_BACKGROUND_FETCH_H_
