// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_EVENTS_APPLICATION_CACHE_ERROR_EVENT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_EVENTS_APPLICATION_CACHE_ERROR_EVENT_H_

#include "third_party/blink/public/platform/web_application_cache_host_client.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/events/application_cache_error_event_init.h"
#include "third_party/blink/renderer/core/loader/appcache/application_cache_host.h"

namespace blink {

class ApplicationCacheErrorEvent final : public Event {
  DEFINE_WRAPPERTYPEINFO();

 public:
  ~ApplicationCacheErrorEvent() override;

  static ApplicationCacheErrorEvent* Create(
      WebApplicationCacheHost::ErrorReason reason,
      const String& url,
      int status,
      const String& message) {
    return new ApplicationCacheErrorEvent(reason, url, status, message);
  }

  static ApplicationCacheErrorEvent* Create(
      const AtomicString& event_type,
      const ApplicationCacheErrorEventInit& initializer) {
    return new ApplicationCacheErrorEvent(event_type, initializer);
  }

  const String& reason() const { return reason_; }
  const String& url() const { return url_; }
  int status() const { return status_; }
  const String& message() const { return message_; }

  const AtomicString& InterfaceName() const override {
    return EventNames::ApplicationCacheErrorEvent;
  }

  void Trace(blink::Visitor*) override;

 private:
  ApplicationCacheErrorEvent(WebApplicationCacheHost::ErrorReason,
                             const String& url,
                             int status,
                             const String& message);
  ApplicationCacheErrorEvent(const AtomicString& event_type,
                             const ApplicationCacheErrorEventInit& initializer);

  String reason_;
  String url_;
  int status_;
  String message_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_EVENTS_APPLICATION_CACHE_ERROR_EVENT_H_
