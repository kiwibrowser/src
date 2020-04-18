/*
 * Copyright (C) 2013 Intel Inc. All rights reserved.
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

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_LOADER_FETCH_RESOURCE_TIMING_INFO_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_LOADER_FETCH_RESOURCE_TIMING_INFO_H_

#include <memory>
#include "third_party/blink/renderer/platform/cross_thread_copier.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_request.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_response.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"
#include "third_party/blink/renderer/platform/wtf/functional.h"
#include "third_party/blink/renderer/platform/wtf/noncopyable.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"
#include "third_party/blink/renderer/platform/wtf/time.h"

namespace blink {

struct CrossThreadResourceTimingInfoData;

class PLATFORM_EXPORT ResourceTimingInfo
    : public RefCounted<ResourceTimingInfo> {
  USING_FAST_MALLOC(ResourceTimingInfo);
  WTF_MAKE_NONCOPYABLE(ResourceTimingInfo);

 public:
  static scoped_refptr<ResourceTimingInfo> Create(const AtomicString& type,
                                                  const TimeTicks time,
                                                  bool is_main_resource) {
    return base::AdoptRef(new ResourceTimingInfo(type, time, is_main_resource));
  }
  static scoped_refptr<ResourceTimingInfo> Adopt(
      std::unique_ptr<CrossThreadResourceTimingInfoData>);

  // Gets a copy of the data suitable for passing to another thread.
  std::unique_ptr<CrossThreadResourceTimingInfoData> CopyData() const;

  TimeTicks InitialTime() const { return initial_time_; }
  bool IsMainResource() const { return is_main_resource_; }

  const AtomicString& InitiatorType() const { return type_; }

  void SetOriginalTimingAllowOrigin(
      const AtomicString& original_timing_allow_origin) {
    original_timing_allow_origin_ = original_timing_allow_origin;
  }
  const AtomicString& OriginalTimingAllowOrigin() const {
    return original_timing_allow_origin_;
  }

  void SetLoadFinishTime(TimeTicks time) { load_finish_time_ = time; }
  TimeTicks LoadFinishTime() const { return load_finish_time_; }

  void SetInitialURL(const KURL& url) { initial_url_ = url; }
  const KURL& InitialURL() const { return initial_url_; }

  void SetFinalResponse(const ResourceResponse& response) {
    final_response_ = response;
  }
  const ResourceResponse& FinalResponse() const { return final_response_; }

  void AddRedirect(const ResourceResponse& redirect_response,
                   bool cross_origin);
  const Vector<ResourceResponse>& RedirectChain() const {
    return redirect_chain_;
  }

  void AddFinalTransferSize(long long encoded_data_length) {
    transfer_size_ += encoded_data_length;
  }
  long long TransferSize() const { return transfer_size_; }

  void ClearLoadTimings() {
    final_response_.SetResourceLoadTiming(nullptr);
    for (ResourceResponse& redirect : redirect_chain_)
      redirect.SetResourceLoadTiming(nullptr);
  }

  // The timestamps in PerformanceResourceTiming are measured relative from the
  // time origin. In most cases these timestamps must be positive value, so we
  // use 0 for invalid negative values. But the timestamps for Service Worker
  // navigation preload requests may be negative, because these requests may
  // be started before the service worker started. We set this flag true, to
  // support such case.
  void SetNegativeAllowed(bool negative_allowed) {
    negative_allowed_ = negative_allowed;
  }
  bool NegativeAllowed() const { return negative_allowed_; }

 private:
  ResourceTimingInfo(const AtomicString& type,
                     const TimeTicks time,
                     bool is_main_resource)
      : type_(type), initial_time_(time), is_main_resource_(is_main_resource) {}

  AtomicString type_;
  AtomicString original_timing_allow_origin_;
  TimeTicks initial_time_;
  TimeTicks load_finish_time_;
  KURL initial_url_;
  ResourceResponse final_response_;
  Vector<ResourceResponse> redirect_chain_;
  long long transfer_size_ = 0;
  bool is_main_resource_;
  bool has_cross_origin_redirect_ = false;
  bool negative_allowed_ = false;
};

struct CrossThreadResourceTimingInfoData {
  WTF_MAKE_NONCOPYABLE(CrossThreadResourceTimingInfoData);
  USING_FAST_MALLOC(CrossThreadResourceTimingInfoData);

 public:
  CrossThreadResourceTimingInfoData() = default;

  String type_;
  String original_timing_allow_origin_;
  TimeTicks initial_time_;
  TimeTicks load_finish_time_;
  KURL initial_url_;
  std::unique_ptr<CrossThreadResourceResponseData> final_response_;
  Vector<std::unique_ptr<CrossThreadResourceResponseData>> redirect_chain_;
  long long transfer_size_;
  bool is_main_resource_;
  bool negative_allowed_;
};

template <>
struct CrossThreadCopier<ResourceTimingInfo> {
  typedef WTF::PassedWrapper<std::unique_ptr<CrossThreadResourceTimingInfoData>>
      Type;
  static Type Copy(const ResourceTimingInfo& info) {
    return WTF::Passed(info.CopyData());
  }
};

}  // namespace blink

#endif
