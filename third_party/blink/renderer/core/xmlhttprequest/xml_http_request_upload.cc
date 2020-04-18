/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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

#include "third_party/blink/renderer/core/xmlhttprequest/xml_http_request_upload.h"

#include "third_party/blink/renderer/core/event_type_names.h"
#include "third_party/blink/renderer/core/events/progress_event.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"

namespace blink {

XMLHttpRequestUpload::XMLHttpRequestUpload(XMLHttpRequest* xml_http_request)
    : xml_http_request_(xml_http_request),
      last_bytes_sent_(0),
      last_total_bytes_to_be_sent_(0) {}

const AtomicString& XMLHttpRequestUpload::InterfaceName() const {
  return EventTargetNames::XMLHttpRequestUpload;
}

ExecutionContext* XMLHttpRequestUpload::GetExecutionContext() const {
  return xml_http_request_->GetExecutionContext();
}

void XMLHttpRequestUpload::DispatchProgressEvent(
    unsigned long long bytes_sent,
    unsigned long long total_bytes_to_be_sent) {
  last_bytes_sent_ = bytes_sent;
  last_total_bytes_to_be_sent_ = total_bytes_to_be_sent;
  probe::AsyncTask async_task(GetExecutionContext(), xml_http_request_,
                              "progress", xml_http_request_->IsAsync());
  DispatchEvent(ProgressEvent::Create(EventTypeNames::progress, true,
                                      bytes_sent, total_bytes_to_be_sent));
}

void XMLHttpRequestUpload::DispatchEventAndLoadEnd(
    const AtomicString& type,
    bool length_computable,
    unsigned long long bytes_sent,
    unsigned long long total) {
  DCHECK(type == EventTypeNames::load || type == EventTypeNames::abort ||
         type == EventTypeNames::error || type == EventTypeNames::timeout);
  probe::AsyncTask async_task(GetExecutionContext(), xml_http_request_, "event",
                              xml_http_request_->IsAsync());
  DispatchEvent(
      ProgressEvent::Create(type, length_computable, bytes_sent, total));
  DispatchEvent(ProgressEvent::Create(EventTypeNames::loadend,
                                      length_computable, bytes_sent, total));
}

void XMLHttpRequestUpload::HandleRequestError(const AtomicString& type) {
  bool length_computable = last_total_bytes_to_be_sent_ > 0 &&
                           last_bytes_sent_ <= last_total_bytes_to_be_sent_;
  probe::AsyncTask async_task(GetExecutionContext(), xml_http_request_, "error",
                              xml_http_request_->IsAsync());
  DispatchEvent(ProgressEvent::Create(EventTypeNames::progress,
                                      length_computable, last_bytes_sent_,
                                      last_total_bytes_to_be_sent_));
  DispatchEventAndLoadEnd(type, length_computable, last_bytes_sent_,
                          last_total_bytes_to_be_sent_);
}

void XMLHttpRequestUpload::Trace(blink::Visitor* visitor) {
  visitor->Trace(xml_http_request_);
  XMLHttpRequestEventTarget::Trace(visitor);
}

}  // namespace blink
