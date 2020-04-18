// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_WEBRTC_RTC_STATS_H_
#define CONTENT_RENDERER_MEDIA_WEBRTC_RTC_STATS_H_

#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "content/common/content_export.h"
#include "third_party/blink/public/platform/web_rtc_stats.h"
#include "third_party/webrtc/api/stats/rtcstats.h"
#include "third_party/webrtc/api/stats/rtcstatscollectorcallback.h"
#include "third_party/webrtc/api/stats/rtcstatsreport.h"

namespace content {

class CONTENT_EXPORT RTCStatsReport : public blink::WebRTCStatsReport {
 public:
  RTCStatsReport(
      const scoped_refptr<const webrtc::RTCStatsReport>& stats_report);
  ~RTCStatsReport() override;
  std::unique_ptr<blink::WebRTCStatsReport> CopyHandle() const override;

  std::unique_ptr<blink::WebRTCStats> GetStats(
      blink::WebString id) const override;
  std::unique_ptr<blink::WebRTCStats> Next() override;
  size_t Size() const override;

 private:
  const scoped_refptr<const webrtc::RTCStatsReport> stats_report_;
  webrtc::RTCStatsReport::ConstIterator it_;
  const webrtc::RTCStatsReport::ConstIterator end_;
};

class CONTENT_EXPORT RTCStats : public blink::WebRTCStats {
 public:
  RTCStats(const scoped_refptr<const webrtc::RTCStatsReport>& stats_owner,
           const webrtc::RTCStats* stats);
  ~RTCStats() override;

  blink::WebString Id() const override;
  blink::WebString GetType() const override;
  double Timestamp() const override;

  size_t MembersCount() const override;
  std::unique_ptr<blink::WebRTCStatsMember> GetMember(size_t i) const override;

 private:
  // Reference to keep the report that owns |stats_| alive.
  const scoped_refptr<const webrtc::RTCStatsReport> stats_owner_;
  // Pointer to a stats object that is owned by |stats_owner_|.
  const webrtc::RTCStats* const stats_;
  // Members of the |stats_| object, equivalent to |stats_->Members()|.
  const std::vector<const webrtc::RTCStatsMemberInterface*> stats_members_;
};

class CONTENT_EXPORT RTCStatsMember : public blink::WebRTCStatsMember {
 public:
  RTCStatsMember(const scoped_refptr<const webrtc::RTCStatsReport>& stats_owner,
                 const webrtc::RTCStatsMemberInterface* member);
  ~RTCStatsMember() override;

  blink::WebString GetName() const override;
  blink::WebRTCStatsMemberType GetType() const override;
  bool IsDefined() const override;

  bool ValueBool() const override;
  int32_t ValueInt32() const override;
  uint32_t ValueUint32() const override;
  int64_t ValueInt64() const override;
  uint64_t ValueUint64() const override;
  double ValueDouble() const override;
  blink::WebString ValueString() const override;
  blink::WebVector<int> ValueSequenceBool() const override;
  blink::WebVector<int32_t> ValueSequenceInt32() const override;
  blink::WebVector<uint32_t> ValueSequenceUint32() const override;
  blink::WebVector<int64_t> ValueSequenceInt64() const override;
  blink::WebVector<uint64_t> ValueSequenceUint64() const override;
  blink::WebVector<double> ValueSequenceDouble() const override;
  blink::WebVector<blink::WebString> ValueSequenceString() const override;

 private:
  // Reference to keep the report that owns |member_|'s stats object alive.
  const scoped_refptr<const webrtc::RTCStatsReport> stats_owner_;
  // Pointer to member of a stats object that is owned by |stats_owner_|.
  const webrtc::RTCStatsMemberInterface* const member_;
};

// A stats collector callback.
// It is invoked on the WebRTC signaling thread and will post a task to invoke
// |callback| on the thread given in the |main_thread| argument.
// The argument to the callback will be a |blink::WebRTCStatsReport|.
class RTCStatsCollectorCallbackImpl : public webrtc::RTCStatsCollectorCallback {
 public:
  static rtc::scoped_refptr<RTCStatsCollectorCallbackImpl> Create(
      scoped_refptr<base::SingleThreadTaskRunner> main_thread,
      std::unique_ptr<blink::WebRTCStatsReportCallback> callback);

  void OnStatsDelivered(
      const rtc::scoped_refptr<const webrtc::RTCStatsReport>& report) override;

 protected:
  RTCStatsCollectorCallbackImpl(
      scoped_refptr<base::SingleThreadTaskRunner> main_thread,
      blink::WebRTCStatsReportCallback* callback);
  ~RTCStatsCollectorCallbackImpl() override;

  void OnStatsDeliveredOnMainThread(
      rtc::scoped_refptr<const webrtc::RTCStatsReport> report);

  const scoped_refptr<base::SingleThreadTaskRunner> main_thread_;
  std::unique_ptr<blink::WebRTCStatsReportCallback> callback_;
};

CONTENT_EXPORT void WhitelistStatsForTesting(const char* type);

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_WEBRTC_RTC_STATS_H_
