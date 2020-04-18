// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/background_fetch/background_fetched_event.h"

#include "third_party/blink/public/platform/modules/background_fetch/web_background_fetch_settled_fetch.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise_resolver.h"
#include "third_party/blink/renderer/core/dom/dom_exception.h"
#include "third_party/blink/renderer/core/fetch/request.h"
#include "third_party/blink/renderer/core/fetch/response.h"
#include "third_party/blink/renderer/modules/background_fetch/background_fetch_bridge.h"
#include "third_party/blink/renderer/modules/background_fetch/background_fetch_settled_fetch.h"
#include "third_party/blink/renderer/modules/background_fetch/background_fetched_event_init.h"
#include "third_party/blink/renderer/modules/event_modules_names.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"

namespace blink {

BackgroundFetchedEvent::BackgroundFetchedEvent(
    const AtomicString& type,
    const BackgroundFetchedEventInit& initializer)
    : BackgroundFetchEvent(type, initializer, nullptr /* observer */),
      fetches_(initializer.fetches()) {}

BackgroundFetchedEvent::BackgroundFetchedEvent(
    const AtomicString& type,
    const BackgroundFetchedEventInit& initializer,
    const String& unique_id,
    const WebVector<WebBackgroundFetchSettledFetch>& fetches,
    ScriptState* script_state,
    WaitUntilObserver* observer,
    ServiceWorkerRegistration* registration)
    : BackgroundFetchEvent(type, initializer, observer),
      unique_id_(unique_id),
      registration_(registration) {
  fetches_.ReserveInitialCapacity(fetches.size());
  for (const WebBackgroundFetchSettledFetch& fetch : fetches) {
    auto* settled_fetch = BackgroundFetchSettledFetch::Create(
        Request::Create(script_state, fetch.request),
        Response::Create(script_state, fetch.response));

    fetches_.push_back(settled_fetch);
  }
}

BackgroundFetchedEvent::~BackgroundFetchedEvent() = default;

HeapVector<Member<BackgroundFetchSettledFetch>>
BackgroundFetchedEvent::fetches() const {
  return fetches_;
}

ScriptPromise BackgroundFetchedEvent::updateUI(ScriptState* script_state,
                                               const String& title) {
  if (!registration_) {
    // Return a Promise that will never settle when a developer calls this
    // method on a BackgroundFetchedEvent instance they created themselves.
    return ScriptPromise();
  }
  DCHECK(!unique_id_.IsEmpty());

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();

  BackgroundFetchBridge::From(registration_)
      ->UpdateUI(id(), unique_id_, title,
                 WTF::Bind(&BackgroundFetchedEvent::DidUpdateUI,
                           WrapPersistent(this), WrapPersistent(resolver)));

  return promise;
}

void BackgroundFetchedEvent::DidUpdateUI(
    ScriptPromiseResolver* resolver,
    mojom::blink::BackgroundFetchError error) {
  switch (error) {
    case mojom::blink::BackgroundFetchError::NONE:
    case mojom::blink::BackgroundFetchError::INVALID_ID:
      resolver->Resolve();
      return;
    case mojom::blink::BackgroundFetchError::STORAGE_ERROR:
      resolver->Reject(DOMException::Create(
          kAbortError, "Failed to update UI due to I/O error."));
      return;
    case mojom::blink::BackgroundFetchError::DUPLICATED_DEVELOPER_ID:
    case mojom::blink::BackgroundFetchError::INVALID_ARGUMENT:
      // Not applicable for this callback.
      break;
  }

  NOTREACHED();
}

const AtomicString& BackgroundFetchedEvent::InterfaceName() const {
  return EventNames::BackgroundFetchedEvent;
}

void BackgroundFetchedEvent::Trace(blink::Visitor* visitor) {
  visitor->Trace(fetches_);
  visitor->Trace(registration_);
  BackgroundFetchEvent::Trace(visitor);
}

}  // namespace blink
