/*
 * Copyright (C) 2008, 2009 Apple Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/loader/appcache/application_cache.h"

#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/events/event_listener.h"
#include "third_party/blink/renderer/core/dom/exception_code.h"
#include "third_party/blink/renderer/core/frame/deprecation.h"
#include "third_party/blink/renderer/core/frame/hosts_using_features.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/use_counter.h"
#include "third_party/blink/renderer/core/loader/document_loader.h"
#include "third_party/blink/renderer/core/loader/frame_loader.h"

namespace blink {

ApplicationCache::ApplicationCache(LocalFrame* frame) : DOMWindowClient(frame) {
  ApplicationCacheHost* cache_host = GetApplicationCacheHost();
  if (cache_host)
    cache_host->SetApplicationCache(this);
}

void ApplicationCache::Trace(blink::Visitor* visitor) {
  EventTargetWithInlineData::Trace(visitor);
  DOMWindowClient::Trace(visitor);
}

ApplicationCacheHost* ApplicationCache::GetApplicationCacheHost() const {
  if (!GetFrame() || !GetFrame()->Loader().GetDocumentLoader())
    return nullptr;
  return GetFrame()->Loader().GetDocumentLoader()->GetApplicationCacheHost();
}

unsigned short ApplicationCache::status() const {
  RecordAPIUseType();
  ApplicationCacheHost* cache_host = GetApplicationCacheHost();
  if (!cache_host)
    return ApplicationCacheHost::kUncached;
  return cache_host->GetStatus();
}

void ApplicationCache::update(ExceptionState& exception_state) {
  RecordAPIUseType();
  ApplicationCacheHost* cache_host = GetApplicationCacheHost();
  if (!cache_host || !cache_host->Update()) {
    exception_state.ThrowDOMException(
        kInvalidStateError, "there is no application cache to update.");
  }
}

void ApplicationCache::swapCache(ExceptionState& exception_state) {
  RecordAPIUseType();
  ApplicationCacheHost* cache_host = GetApplicationCacheHost();
  if (!cache_host || !cache_host->SwapCache()) {
    exception_state.ThrowDOMException(
        kInvalidStateError, "there is no newer application cache to swap to.");
  }
}

void ApplicationCache::abort() {
  ApplicationCacheHost* cache_host = GetApplicationCacheHost();
  if (cache_host)
    cache_host->Abort();
}

const AtomicString& ApplicationCache::InterfaceName() const {
  return EventTargetNames::ApplicationCache;
}

ExecutionContext* ApplicationCache::GetExecutionContext() const {
  return GetFrame() ? GetFrame()->GetDocument() : nullptr;
}

const AtomicString& ApplicationCache::ToEventType(
    ApplicationCacheHost::EventID id) {
  switch (id) {
    case ApplicationCacheHost::kCheckingEvent:
      return EventTypeNames::checking;
    case ApplicationCacheHost::kErrorEvent:
      return EventTypeNames::error;
    case ApplicationCacheHost::kNoupdateEvent:
      return EventTypeNames::noupdate;
    case ApplicationCacheHost::kDownloadingEvent:
      return EventTypeNames::downloading;
    case ApplicationCacheHost::kProgressEvent:
      return EventTypeNames::progress;
    case ApplicationCacheHost::kUpdatereadyEvent:
      return EventTypeNames::updateready;
    case ApplicationCacheHost::kCachedEvent:
      return EventTypeNames::cached;
    case ApplicationCacheHost::kObsoleteEvent:
      return EventTypeNames::obsolete;
  }
  NOTREACHED();
  return EventTypeNames::error;
}

void ApplicationCache::RecordAPIUseType() const {
  if (!GetFrame())
    return;

  Document* document = GetFrame()->GetDocument();

  if (!document)
    return;

  if (document->IsSecureContext()) {
    UseCounter::Count(document, WebFeature::kApplicationCacheAPISecureOrigin);
  } else {
    Deprecation::CountDeprecation(
        document, WebFeature::kApplicationCacheAPIInsecureOrigin);
    HostsUsingFeatures::CountAnyWorld(
        *document,
        HostsUsingFeatures::Feature::kApplicationCacheAPIInsecureHost);
  }
}

}  // namespace blink
