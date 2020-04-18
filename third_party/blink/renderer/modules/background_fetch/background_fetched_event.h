// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_BACKGROUND_FETCH_BACKGROUND_FETCHED_EVENT_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_BACKGROUND_FETCH_BACKGROUND_FETCHED_EVENT_H_

#include "third_party/blink/public/platform/modules/background_fetch/background_fetch.mojom-blink.h"
#include "third_party/blink/public/platform/web_vector.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise.h"
#include "third_party/blink/renderer/modules/background_fetch/background_fetch_event.h"
#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"

namespace blink {

class BackgroundFetchSettledFetch;
class BackgroundFetchedEventInit;
class ScriptState;
class ServiceWorkerRegistration;
struct WebBackgroundFetchSettledFetch;

class MODULES_EXPORT BackgroundFetchedEvent final
    : public BackgroundFetchEvent {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static BackgroundFetchedEvent* Create(
      const AtomicString& type,
      const BackgroundFetchedEventInit& initializer) {
    return new BackgroundFetchedEvent(type, initializer);
  }

  static BackgroundFetchedEvent* Create(
      const AtomicString& type,
      const BackgroundFetchedEventInit& initializer,
      const String& unique_id,
      const WebVector<WebBackgroundFetchSettledFetch>& fetches,
      ScriptState* script_state,
      WaitUntilObserver* observer,
      ServiceWorkerRegistration* registration) {
    return new BackgroundFetchedEvent(type, initializer, unique_id, fetches,
                                      script_state, observer, registration);
  }

  ~BackgroundFetchedEvent() override;

  // Web Exposed attribute defined in the IDL file.
  HeapVector<Member<BackgroundFetchSettledFetch>> fetches() const;

  // Web Exposed method defined in the IDL file.
  ScriptPromise updateUI(ScriptState* script_state, const String& title);

  // ExtendableEvent interface.
  const AtomicString& InterfaceName() const override;

  void Trace(blink::Visitor* visitor) override;

 private:
  BackgroundFetchedEvent(const AtomicString& type,
                         const BackgroundFetchedEventInit& initializer);
  BackgroundFetchedEvent(
      const AtomicString& type,
      const BackgroundFetchedEventInit& initializer,
      const String& unique_id,
      const WebVector<WebBackgroundFetchSettledFetch>& fetches,
      ScriptState* script_state,
      WaitUntilObserver* observer,
      ServiceWorkerRegistration* registration);

  void DidUpdateUI(ScriptPromiseResolver* resolver,
                   mojom::blink::BackgroundFetchError error);

  // Globally unique ID for the registration, generated in content/. Used to
  // distinguish registrations in case a developer re-uses |developer_id_|s. Not
  // exposed to JavaScript.
  String unique_id_;

  HeapVector<Member<BackgroundFetchSettledFetch>> fetches_;
  Member<ServiceWorkerRegistration> registration_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_BACKGROUND_FETCH_BACKGROUND_FETCHED_EVENT_H_
