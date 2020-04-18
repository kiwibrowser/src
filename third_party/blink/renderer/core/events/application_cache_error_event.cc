// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/events/application_cache_error_event.h"

namespace blink {

static const String& ErrorReasonToString(
    WebApplicationCacheHost::ErrorReason reason) {
  DEFINE_STATIC_LOCAL(String, error_manifest, ("manifest"));
  DEFINE_STATIC_LOCAL(String, error_signature, ("signature"));
  DEFINE_STATIC_LOCAL(String, error_resource, ("resource"));
  DEFINE_STATIC_LOCAL(String, error_changed, ("changed"));
  DEFINE_STATIC_LOCAL(String, error_abort, ("abort"));
  DEFINE_STATIC_LOCAL(String, error_quota, ("quota"));
  DEFINE_STATIC_LOCAL(String, error_policy, ("policy"));
  DEFINE_STATIC_LOCAL(String, error_unknown, ("unknown"));

  switch (reason) {
    case WebApplicationCacheHost::kManifestError:
      return error_manifest;
    case WebApplicationCacheHost::kSignatureError:
      return error_signature;
    case WebApplicationCacheHost::kResourceError:
      return error_resource;
    case WebApplicationCacheHost::kChangedError:
      return error_changed;
    case WebApplicationCacheHost::kAbortError:
      return error_abort;
    case WebApplicationCacheHost::kQuotaError:
      return error_quota;
    case WebApplicationCacheHost::kPolicyError:
      return error_policy;
    case WebApplicationCacheHost::kUnknownError:
      return error_unknown;
  }
  NOTREACHED();
  return g_empty_string;
}

ApplicationCacheErrorEvent::ApplicationCacheErrorEvent(
    WebApplicationCacheHost::ErrorReason reason,
    const String& url,
    int status,
    const String& message)
    : Event(EventTypeNames::error, Bubbles::kNo, Cancelable::kNo),
      reason_(ErrorReasonToString(reason)),
      url_(url),
      status_(status),
      message_(message) {}

ApplicationCacheErrorEvent::ApplicationCacheErrorEvent(
    const AtomicString& event_type,
    const ApplicationCacheErrorEventInit& initializer)
    : Event(event_type, initializer), status_(0) {
  if (initializer.hasReason())
    reason_ = initializer.reason();
  if (initializer.hasURL())
    url_ = initializer.url();
  if (initializer.hasStatus())
    status_ = initializer.status();
  if (initializer.hasMessage())
    message_ = initializer.message();
}

ApplicationCacheErrorEvent::~ApplicationCacheErrorEvent() = default;

void ApplicationCacheErrorEvent::Trace(blink::Visitor* visitor) {
  Event::Trace(visitor);
}

}  // namespace blink
