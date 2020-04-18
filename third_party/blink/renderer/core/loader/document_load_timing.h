/*
 * Copyright (C) 2010 Google, Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY GOOGLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GOOGLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_DOCUMENT_LOAD_TIMING_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_DOCUMENT_LOAD_TIMING_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/traced_value.h"
#include "third_party/blink/renderer/platform/wtf/time.h"

namespace blink {

class DocumentLoader;
class KURL;
class LocalFrame;

class CORE_EXPORT DocumentLoadTiming final {
  DISALLOW_NEW();

 public:
  explicit DocumentLoadTiming(DocumentLoader&);

  double MonotonicTimeToZeroBasedDocumentTime(TimeTicks) const;
  double MonotonicTimeToPseudoWallTime(TimeTicks) const;
  TimeTicks PseudoWallTimeToMonotonicTime(double) const;

  void MarkNavigationStart();
  void SetNavigationStart(TimeTicks);

  void AddRedirect(const KURL& redirecting_url, const KURL& redirected_url);
  void SetRedirectStart(TimeTicks);
  void SetRedirectEnd(TimeTicks);
  void SetRedirectCount(short value) { redirect_count_ = value; }
  void SetHasCrossOriginRedirect(bool value) {
    has_cross_origin_redirect_ = value;
  }

  void MarkUnloadEventStart(TimeTicks);
  void MarkUnloadEventEnd(TimeTicks);

  void MarkFetchStart();
  void SetFetchStart(TimeTicks);

  void SetResponseEnd(TimeTicks);

  void MarkLoadEventStart();
  void MarkLoadEventEnd();

  void SetHasSameOriginAsPreviousDocument(bool value) {
    has_same_origin_as_previous_document_ = value;
  }

  TimeTicks NavigationStart() const { return navigation_start_; }
  TimeTicks UnloadEventStart() const { return unload_event_start_; }
  TimeTicks UnloadEventEnd() const { return unload_event_end_; }
  TimeTicks RedirectStart() const { return redirect_start_; }
  TimeTicks RedirectEnd() const { return redirect_end_; }
  short RedirectCount() const { return redirect_count_; }
  TimeTicks FetchStart() const { return fetch_start_; }
  TimeTicks ResponseEnd() const { return response_end_; }
  TimeTicks LoadEventStart() const { return load_event_start_; }
  TimeTicks LoadEventEnd() const { return load_event_end_; }
  bool HasCrossOriginRedirect() const { return has_cross_origin_redirect_; }
  bool HasSameOriginAsPreviousDocument() const {
    return has_same_origin_as_previous_document_;
  }

  TimeTicks ReferenceMonotonicTime() const { return reference_monotonic_time_; }

  void Trace(blink::Visitor*);

 private:
  void MarkRedirectEnd();
  void NotifyDocumentTimingChanged();
  void EnsureReferenceTimesSet();
  LocalFrame* GetFrame() const;
  std::unique_ptr<TracedValue> GetNavigationStartTracingData() const;

  TimeTicks reference_monotonic_time_;
  double reference_wall_time_;
  TimeTicks navigation_start_;
  TimeTicks unload_event_start_;
  TimeTicks unload_event_end_;
  TimeTicks redirect_start_;
  TimeTicks redirect_end_;
  short redirect_count_;
  TimeTicks fetch_start_;
  TimeTicks response_end_;
  TimeTicks load_event_start_;
  TimeTicks load_event_end_;
  bool has_cross_origin_redirect_;
  bool has_same_origin_as_previous_document_;

  Member<DocumentLoader> document_loader_;
};

}  // namespace blink

#endif
