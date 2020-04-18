// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_BACKGROUND_FETCH_BACKGROUND_FETCH_SETTLED_EVENT_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_BACKGROUND_FETCH_BACKGROUND_FETCH_SETTLED_EVENT_H_

#include "third_party/blink/renderer/modules/background_fetch/background_fetch_event.h"
#include "third_party/blink/renderer/modules/background_fetch/background_fetch_settled_event_init.h"
#include "third_party/blink/renderer/modules/background_fetch/background_fetch_settled_fetches.h"
#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/heap/member.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"

namespace blink {

// Event for interacting with fetch requests that have completed.
class MODULES_EXPORT BackgroundFetchSettledEvent : public BackgroundFetchEvent {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static BackgroundFetchSettledEvent* Create(
      const AtomicString& type,
      const BackgroundFetchSettledEventInit& initializer) {
    return new BackgroundFetchSettledEvent(type, initializer);
  }

  static BackgroundFetchSettledEvent* Create(
      const AtomicString& type,
      const BackgroundFetchSettledEventInit& initializer,
      const String& unique_id,
      WaitUntilObserver* observer = nullptr) {
    return new BackgroundFetchSettledEvent(type, initializer, unique_id,
                                           observer);
  }

  ~BackgroundFetchSettledEvent() override;

  // Web Exposed attribute defined in the IDL file.
  BackgroundFetchSettledFetches* fetches() const;

  void Trace(blink::Visitor*) override;

 protected:
  BackgroundFetchSettledEvent(
      const AtomicString& type,
      const BackgroundFetchSettledEventInit& initializer,
      const String& unique_id,
      WaitUntilObserver* observer = nullptr);

  BackgroundFetchSettledEvent(
      const AtomicString& type,
      const BackgroundFetchSettledEventInit& initializer);

  // Globally unique ID for the registration, generated in content/. Used to
  // distinguish registrations in case a developer re-uses |developer_id_|s. Not
  // exposed to JavaScript.
  String unique_id_;

 private:
  Member<BackgroundFetchSettledFetches> fetches_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_BACKGROUND_FETCH_BACKGROUND_FETCH_SETTLED_EVENT_H_
