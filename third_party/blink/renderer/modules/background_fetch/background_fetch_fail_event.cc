// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/background_fetch/background_fetch_fail_event.h"

#include "third_party/blink/public/platform/modules/background_fetch/web_background_fetch_settled_fetch.h"
#include "third_party/blink/renderer/core/fetch/request.h"
#include "third_party/blink/renderer/core/fetch/response.h"
#include "third_party/blink/renderer/modules/background_fetch/background_fetch_fail_event_init.h"
#include "third_party/blink/renderer/modules/background_fetch/background_fetch_settled_fetch.h"
#include "third_party/blink/renderer/modules/event_modules_names.h"

namespace blink {

BackgroundFetchFailEvent::BackgroundFetchFailEvent(
    const AtomicString& type,
    const BackgroundFetchFailEventInit& initializer)
    : BackgroundFetchEvent(type, initializer, nullptr /* observer */),
      fetches_(initializer.fetches()) {}

BackgroundFetchFailEvent::BackgroundFetchFailEvent(
    const AtomicString& type,
    const BackgroundFetchFailEventInit& initializer,
    const WebVector<WebBackgroundFetchSettledFetch>& fetches,
    ScriptState* script_state,
    WaitUntilObserver* observer)
    : BackgroundFetchEvent(type, initializer, observer) {
  fetches_.ReserveInitialCapacity(fetches.size());
  for (const WebBackgroundFetchSettledFetch& fetch : fetches) {
    auto* settled_fetch = BackgroundFetchSettledFetch::Create(
        Request::Create(script_state, fetch.request),
        Response::Create(script_state, fetch.response));

    fetches_.push_back(settled_fetch);
  }
}

BackgroundFetchFailEvent::~BackgroundFetchFailEvent() = default;

HeapVector<Member<BackgroundFetchSettledFetch>>
BackgroundFetchFailEvent::fetches() const {
  return fetches_;
}

const AtomicString& BackgroundFetchFailEvent::InterfaceName() const {
  return EventNames::BackgroundFetchFailEvent;
}

void BackgroundFetchFailEvent::Trace(blink::Visitor* visitor) {
  visitor->Trace(fetches_);
  BackgroundFetchEvent::Trace(visitor);
}

}  // namespace blink
