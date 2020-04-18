// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/notifications/notification_event.h"

#include "third_party/blink/renderer/modules/notifications/notification_event_init.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

NotificationEvent::NotificationEvent(const AtomicString& type,
                                     const NotificationEventInit& initializer)
    : ExtendableEvent(type, initializer),
      action_(initializer.action()),
      reply_(initializer.reply()) {
  if (initializer.hasNotification())
    notification_ = initializer.notification();
}

NotificationEvent::NotificationEvent(const AtomicString& type,
                                     const NotificationEventInit& initializer,
                                     WaitUntilObserver* observer)
    : ExtendableEvent(type, initializer, observer),
      action_(initializer.action()),
      reply_(initializer.reply()) {
  if (initializer.hasNotification())
    notification_ = initializer.notification();
}

NotificationEvent::~NotificationEvent() = default;

const AtomicString& NotificationEvent::InterfaceName() const {
  return EventNames::NotificationEvent;
}

void NotificationEvent::Trace(blink::Visitor* visitor) {
  visitor->Trace(notification_);
  ExtendableEvent::Trace(visitor);
}

}  // namespace blink
