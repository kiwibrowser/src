// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/background_fetch/background_fetch_event.h"

#include "third_party/blink/renderer/modules/background_fetch/background_fetch_event_init.h"
#include "third_party/blink/renderer/modules/event_modules_names.h"

namespace blink {

BackgroundFetchEvent::BackgroundFetchEvent(
    const AtomicString& type,
    const BackgroundFetchEventInit& initializer,
    WaitUntilObserver* observer)
    : ExtendableEvent(type, initializer, observer),
      developer_id_(initializer.id()) {}

BackgroundFetchEvent::~BackgroundFetchEvent() = default;

String BackgroundFetchEvent::id() const {
  return developer_id_;
}

const AtomicString& BackgroundFetchEvent::InterfaceName() const {
  return EventNames::BackgroundFetchEvent;
}

}  // namespace blink
