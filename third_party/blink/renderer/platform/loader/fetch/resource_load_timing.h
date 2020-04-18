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

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_LOADER_FETCH_RESOURCE_LOAD_TIMING_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_LOADER_FETCH_RESOURCE_LOAD_TIMING_H_

#include "base/memory/scoped_refptr.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/ref_counted.h"
#include "third_party/blink/renderer/platform/wtf/time.h"

namespace blink {

class PLATFORM_EXPORT ResourceLoadTiming
    : public RefCounted<ResourceLoadTiming> {
 public:
  static scoped_refptr<ResourceLoadTiming> Create();

  scoped_refptr<ResourceLoadTiming> DeepCopy();
  bool operator==(const ResourceLoadTiming&) const;
  bool operator!=(const ResourceLoadTiming&) const;

  void SetDnsStart(TimeTicks);
  void SetRequestTime(TimeTicks);
  void SetProxyStart(TimeTicks);
  void SetProxyEnd(TimeTicks);
  void SetDnsEnd(TimeTicks);
  void SetConnectStart(TimeTicks);
  void SetConnectEnd(TimeTicks);
  void SetWorkerStart(TimeTicks);
  void SetWorkerReady(TimeTicks);
  void SetSendStart(TimeTicks);
  void SetSendEnd(TimeTicks);
  void SetReceiveHeadersEnd(TimeTicks);
  void SetSslStart(TimeTicks);
  void SetSslEnd(TimeTicks);
  void SetPushStart(TimeTicks);
  void SetPushEnd(TimeTicks);

  TimeTicks DnsStart() const { return dns_start_; }
  TimeTicks RequestTime() const { return request_time_; }
  TimeTicks ProxyStart() const { return proxy_start_; }
  TimeTicks ProxyEnd() const { return proxy_end_; }
  TimeTicks DnsEnd() const { return dns_end_; }
  TimeTicks ConnectStart() const { return connect_start_; }
  TimeTicks ConnectEnd() const { return connect_end_; }
  TimeTicks WorkerStart() const { return worker_start_; }
  TimeTicks WorkerReady() const { return worker_ready_; }
  TimeTicks SendStart() const { return send_start_; }
  TimeTicks SendEnd() const { return send_end_; }
  TimeTicks ReceiveHeadersEnd() const { return receive_headers_end_; }
  TimeTicks SslStart() const { return ssl_start_; }
  TimeTicks SslEnd() const { return ssl_end_; }
  TimeTicks PushStart() const { return push_start_; }
  TimeTicks PushEnd() const { return push_end_; }

  double CalculateMillisecondDelta(TimeTicks) const;

 private:
  ResourceLoadTiming();

  // We want to present a unified timeline to Javascript. Using walltime is
  // problematic, because the clock may skew while resources load. To prevent
  // that skew, we record a single reference walltime when root document
  // navigation begins. All other times are recorded using
  // monotonicallyIncreasingTime(). When a time needs to be presented to
  // Javascript, we build a pseudo-walltime using the following equation
  // (m_requestTime as example):
  //   pseudo time = document wall reference +
  //                     (m_requestTime - document monotonic reference).

  // All values from monotonicallyIncreasingTime(), in WTF::TimeTicks.
  TimeTicks request_time_;
  TimeTicks proxy_start_;
  TimeTicks proxy_end_;
  TimeTicks dns_start_;
  TimeTicks dns_end_;
  TimeTicks connect_start_;
  TimeTicks connect_end_;
  TimeTicks worker_start_;
  TimeTicks worker_ready_;
  TimeTicks send_start_;
  TimeTicks send_end_;
  TimeTicks receive_headers_end_;
  TimeTicks ssl_start_;
  TimeTicks ssl_end_;
  TimeTicks push_start_;
  TimeTicks push_end_;
};

}  // namespace blink

#endif
