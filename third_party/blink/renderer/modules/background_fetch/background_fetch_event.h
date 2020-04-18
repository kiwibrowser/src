// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_BACKGROUND_FETCH_BACKGROUND_FETCH_EVENT_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_BACKGROUND_FETCH_BACKGROUND_FETCH_EVENT_H_

#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/modules/serviceworkers/extendable_event.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"

namespace blink {

class BackgroundFetchEventInit;
class WaitUntilObserver;

class MODULES_EXPORT BackgroundFetchEvent : public ExtendableEvent {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static BackgroundFetchEvent* Create(
      const AtomicString& type,
      const BackgroundFetchEventInit& initializer) {
    return new BackgroundFetchEvent(type, initializer, nullptr /* observer */);
  }

  static BackgroundFetchEvent* Create(
      const AtomicString& type,
      const BackgroundFetchEventInit& initializer,
      WaitUntilObserver* observer) {
    return new BackgroundFetchEvent(type, initializer, observer);
  }

  ~BackgroundFetchEvent() override;

  // Web Exposed attribute defined in the IDL file. Corresponds to the
  // |developer_id| used elsewhere in the codebase.
  String id() const;

  // ExtendableEvent interface.
  const AtomicString& InterfaceName() const override;

 protected:
  BackgroundFetchEvent(const AtomicString& type,
                       const BackgroundFetchEventInit& initializer,
                       WaitUntilObserver* observer);

  // Corresponds to IDL 'id' attribute. Not unique - an active registration can
  // have the same |developer_id_| as one or more inactive registrations.
  String developer_id_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_BACKGROUND_FETCH_BACKGROUND_FETCH_EVENT_H_
