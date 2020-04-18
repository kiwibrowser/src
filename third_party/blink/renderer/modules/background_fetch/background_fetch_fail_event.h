// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_BACKGROUND_FETCH_BACKGROUND_FETCH_FAIL_EVENT_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_BACKGROUND_FETCH_BACKGROUND_FETCH_FAIL_EVENT_H_

#include "third_party/blink/public/platform/web_vector.h"
#include "third_party/blink/renderer/modules/background_fetch/background_fetch_event.h"
#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"

namespace blink {

class BackgroundFetchFailEventInit;
class BackgroundFetchSettledFetch;
class ScriptState;
struct WebBackgroundFetchSettledFetch;

class MODULES_EXPORT BackgroundFetchFailEvent final
    : public BackgroundFetchEvent {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static BackgroundFetchFailEvent* Create(
      const AtomicString& type,
      const BackgroundFetchFailEventInit& initializer) {
    return new BackgroundFetchFailEvent(type, initializer);
  }

  static BackgroundFetchFailEvent* Create(
      const AtomicString& type,
      const BackgroundFetchFailEventInit& initializer,
      const WebVector<WebBackgroundFetchSettledFetch>& fetches,
      ScriptState* script_state,
      WaitUntilObserver* observer) {
    return new BackgroundFetchFailEvent(type, initializer, fetches,
                                        script_state, observer);
  }

  ~BackgroundFetchFailEvent() override;

  // Web Exposed attribute defined in the IDL file.
  HeapVector<Member<BackgroundFetchSettledFetch>> fetches() const;

  // ExtendableEvent interface.
  const AtomicString& InterfaceName() const override;

  void Trace(blink::Visitor* visitor) override;

 private:
  BackgroundFetchFailEvent(const AtomicString& type,
                           const BackgroundFetchFailEventInit& initializer);
  BackgroundFetchFailEvent(
      const AtomicString& type,
      const BackgroundFetchFailEventInit& initializer,
      const WebVector<WebBackgroundFetchSettledFetch>& fetches,
      ScriptState* script_state,
      WaitUntilObserver* observer);

  HeapVector<Member<BackgroundFetchSettledFetch>> fetches_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_BACKGROUND_FETCH_BACKGROUND_FETCH_FAIL_EVENT_H_
