// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_BACKGROUND_FETCH_BACKGROUND_FETCH_UPDATE_EVENT_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_BACKGROUND_FETCH_BACKGROUND_FETCH_UPDATE_EVENT_H_

#include "third_party/blink/public/platform/modules/background_fetch/background_fetch.mojom-blink.h"
#include "third_party/blink/renderer/modules/background_fetch/background_fetch_settled_event.h"
#include "third_party/blink/renderer/modules/serviceworkers/service_worker_registration.h"

namespace blink {

// Event for interacting with fetch requests that have completed.
class MODULES_EXPORT BackgroundFetchUpdateEvent final
    : public BackgroundFetchSettledEvent {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static BackgroundFetchUpdateEvent* Create(
      const AtomicString& type,
      const BackgroundFetchSettledEventInit& initializer) {
    return new BackgroundFetchUpdateEvent(type, initializer);
  }

  static BackgroundFetchUpdateEvent* Create(
      const AtomicString& type,
      const BackgroundFetchSettledEventInit& initializer,
      const String& unique_id,
      ScriptState* script_state,
      WaitUntilObserver* observer,
      ServiceWorkerRegistration* registration) {
    return new BackgroundFetchUpdateEvent(type, initializer, unique_id,
                                          script_state, observer, registration);
  }

  ~BackgroundFetchUpdateEvent() override;

  // Web Exposed method defined in the IDL file.
  ScriptPromise updateUI(ScriptState* script_state, const String& title);

  void Trace(blink::Visitor* visitor) override;

 private:
  BackgroundFetchUpdateEvent(
      const AtomicString& type,
      const BackgroundFetchSettledEventInit& initializer);

  BackgroundFetchUpdateEvent(const AtomicString& type,
                             const BackgroundFetchSettledEventInit&,
                             const String& unique_id,
                             ScriptState* script_state,
                             WaitUntilObserver* observer,
                             ServiceWorkerRegistration* registration);

  void DidUpdateUI(ScriptPromiseResolver* resolver,
                   mojom::blink::BackgroundFetchError error);

  Member<ServiceWorkerRegistration> registration_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_BACKGROUND_FETCH_BACKGROUND_FETCH_UPDATE_EVENT_H_
