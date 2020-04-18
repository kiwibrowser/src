// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_BACKGROUND_FETCH_BACKGROUND_FETCH_CLICK_EVENT_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_BACKGROUND_FETCH_BACKGROUND_FETCH_CLICK_EVENT_H_

#include "third_party/blink/renderer/modules/background_fetch/background_fetch_event.h"
#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"

namespace blink {

class BackgroundFetchClickEventInit;
class WaitUntilObserver;

class MODULES_EXPORT BackgroundFetchClickEvent final
    : public BackgroundFetchEvent {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static BackgroundFetchClickEvent* Create(
      const AtomicString& type,
      const BackgroundFetchClickEventInit& initializer) {
    return new BackgroundFetchClickEvent(type, initializer,
                                         nullptr /* observer */);
  }

  static BackgroundFetchClickEvent* Create(
      const AtomicString& type,
      const BackgroundFetchClickEventInit& initializer,
      WaitUntilObserver* observer) {
    return new BackgroundFetchClickEvent(type, initializer, observer);
  }

  ~BackgroundFetchClickEvent() override;

  // Web Exposed attribute defined in the IDL file.
  AtomicString state() const;

  // ExtendableEvent interface.
  const AtomicString& InterfaceName() const override;

 private:
  BackgroundFetchClickEvent(const AtomicString& type,
                            const BackgroundFetchClickEventInit& initializer,
                            WaitUntilObserver* observer);

  AtomicString state_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_BACKGROUND_FETCH_BACKGROUND_FETCH_CLICK_EVENT_H_
