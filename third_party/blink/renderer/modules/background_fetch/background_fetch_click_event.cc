// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/background_fetch/background_fetch_click_event.h"

#include "third_party/blink/renderer/modules/background_fetch/background_fetch_click_event_init.h"
#include "third_party/blink/renderer/modules/event_modules_names.h"

namespace blink {

BackgroundFetchClickEvent::BackgroundFetchClickEvent(
    const AtomicString& type,
    const BackgroundFetchClickEventInit& initializer,
    WaitUntilObserver* observer)
    : BackgroundFetchEvent(type, initializer, observer),
      state_(initializer.state()) {}

BackgroundFetchClickEvent::~BackgroundFetchClickEvent() = default;

AtomicString BackgroundFetchClickEvent::state() const {
  return state_;
}

const AtomicString& BackgroundFetchClickEvent::InterfaceName() const {
  return EventNames::BackgroundFetchClickEvent;
}

}  // namespace blink
