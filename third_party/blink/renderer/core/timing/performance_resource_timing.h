/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 * Copyright (C) 2012 Intel Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_TIMING_PERFORMANCE_RESOURCE_TIMING_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_TIMING_PERFORMANCE_RESOURCE_TIMING_H_

#include "third_party/blink/renderer/core/dom/dom_high_res_time_stamp.h"
#include "third_party/blink/renderer/core/timing/performance_entry.h"
#include "third_party/blink/renderer/core/timing/performance_server_timing.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"
#include "third_party/blink/renderer/platform/wtf/time.h"

namespace blink {

class ResourceLoadTiming;
struct WebResourceTimingInfo;

class CORE_EXPORT PerformanceResourceTiming : public PerformanceEntry {
  DEFINE_WRAPPERTYPEINFO();
  friend class PerformanceResourceTimingTest;

 public:
  ~PerformanceResourceTiming() override;
  static PerformanceResourceTiming* Create(
      const WebResourceTimingInfo& info,
      TimeTicks time_origin,
      const AtomicString& initiator_type = g_null_atom) {
    return new PerformanceResourceTiming(info, time_origin, initiator_type);
  }

  // Related doc: https://goo.gl/uNecAj.
  virtual AtomicString initiatorType() const;
  AtomicString nextHopProtocol() const;
  DOMHighResTimeStamp workerStart() const;
  virtual DOMHighResTimeStamp redirectStart() const;
  virtual DOMHighResTimeStamp redirectEnd() const;
  virtual DOMHighResTimeStamp fetchStart() const;
  DOMHighResTimeStamp domainLookupStart() const;
  DOMHighResTimeStamp domainLookupEnd() const;
  DOMHighResTimeStamp connectStart() const;
  DOMHighResTimeStamp connectEnd() const;
  DOMHighResTimeStamp secureConnectionStart() const;
  DOMHighResTimeStamp requestStart() const;
  DOMHighResTimeStamp responseStart() const;
  virtual DOMHighResTimeStamp responseEnd() const;
  unsigned long long transferSize() const;
  unsigned long long encodedBodySize() const;
  unsigned long long decodedBodySize() const;
  const HeapVector<Member<PerformanceServerTiming>>& serverTiming() const;

  void Trace(blink::Visitor*) override;

 protected:
  void BuildJSONValue(V8ObjectBuilder&) const override;

  // This constructor is for PerformanceNavigationTiming.
  // Related doc: https://goo.gl/uNecAj.
  PerformanceResourceTiming(const String& name,
                            const String& entry_type,
                            TimeTicks time_origin,
                            const WebVector<WebServerTimingInfo>&);
  virtual AtomicString AlpnNegotiatedProtocol() const;
  virtual AtomicString ConnectionInfo() const;

 protected:
  TimeTicks TimeOrigin() const { return time_origin_; }

 private:
  PerformanceResourceTiming(const WebResourceTimingInfo&,
                            TimeTicks time_origin,
                            const AtomicString& initiator_type);

  static AtomicString GetNextHopProtocol(
      const AtomicString& alpn_negotiated_protocol,
      const AtomicString& connection_info);

  double WorkerReady() const;

  virtual ResourceLoadTiming* GetResourceLoadTiming() const;
  virtual bool AllowTimingDetails() const;
  virtual bool DidReuseConnection() const;
  virtual unsigned long long GetTransferSize() const;
  virtual unsigned long long GetEncodedBodySize() const;
  virtual unsigned long long GetDecodedBodySize() const;

  AtomicString initiator_type_;
  AtomicString alpn_negotiated_protocol_;
  AtomicString connection_info_;
  TimeTicks time_origin_;
  scoped_refptr<ResourceLoadTiming> timing_;
  TimeTicks last_redirect_end_time_;
  TimeTicks finish_time_;
  unsigned long long transfer_size_;
  unsigned long long encoded_body_size_;
  unsigned long long decoded_body_size_;
  bool did_reuse_connection_;
  bool allow_timing_details_;
  bool allow_redirect_details_;
  bool allow_negative_value_;
  HeapVector<Member<PerformanceServerTiming>> server_timing_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_TIMING_PERFORMANCE_RESOURCE_TIMING_H_
