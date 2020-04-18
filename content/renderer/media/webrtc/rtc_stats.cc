// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/webrtc/rtc_stats.h"

#include <set>
#include <string>

#include "base/bind.h"
#include "base/logging.h"
#include "base/time/time.h"
#include "third_party/webrtc/api/stats/rtcstats_objects.h"

namespace content {

namespace {

class RTCStatsWhitelist {
 public:
  RTCStatsWhitelist() {
    whitelisted_stats_types_.insert(webrtc::RTCCertificateStats::kType);
    whitelisted_stats_types_.insert(webrtc::RTCCodecStats::kType);
    whitelisted_stats_types_.insert(webrtc::RTCDataChannelStats::kType);
    whitelisted_stats_types_.insert(webrtc::RTCIceCandidatePairStats::kType);
    whitelisted_stats_types_.insert(webrtc::RTCIceCandidateStats::kType);
    whitelisted_stats_types_.insert(webrtc::RTCLocalIceCandidateStats::kType);
    whitelisted_stats_types_.insert(webrtc::RTCRemoteIceCandidateStats::kType);
    whitelisted_stats_types_.insert(webrtc::RTCMediaStreamStats::kType);
    whitelisted_stats_types_.insert(webrtc::RTCMediaStreamTrackStats::kType);
    whitelisted_stats_types_.insert(webrtc::RTCPeerConnectionStats::kType);
    whitelisted_stats_types_.insert(webrtc::RTCRTPStreamStats::kType);
    whitelisted_stats_types_.insert(webrtc::RTCInboundRTPStreamStats::kType);
    whitelisted_stats_types_.insert(webrtc::RTCOutboundRTPStreamStats::kType);
    whitelisted_stats_types_.insert(webrtc::RTCTransportStats::kType);
  }

  bool IsWhitelisted(const webrtc::RTCStats& stats) {
    return whitelisted_stats_types_.find(stats.type()) !=
           whitelisted_stats_types_.end();
  }

  void WhitelistStatsForTesting(const char* type) {
    whitelisted_stats_types_.insert(type);
  }

 private:
  std::set<std::string> whitelisted_stats_types_;
};

RTCStatsWhitelist* GetStatsWhitelist() {
  static RTCStatsWhitelist* whitelist = new RTCStatsWhitelist();
  return whitelist;
}

bool IsWhitelistedStats(const webrtc::RTCStats& stats) {
  return GetStatsWhitelist()->IsWhitelisted(stats);
}

}  // namespace

RTCStatsReport::RTCStatsReport(
    const scoped_refptr<const webrtc::RTCStatsReport>& stats_report)
    : stats_report_(stats_report),
      it_(stats_report_->begin()),
      end_(stats_report_->end()) {
  DCHECK(stats_report_);
}

RTCStatsReport::~RTCStatsReport() {
}

std::unique_ptr<blink::WebRTCStatsReport> RTCStatsReport::CopyHandle() const {
  return std::unique_ptr<blink::WebRTCStatsReport>(
      new RTCStatsReport(stats_report_));
}

std::unique_ptr<blink::WebRTCStats> RTCStatsReport::GetStats(
    blink::WebString id) const {
  const webrtc::RTCStats* stats = stats_report_->Get(id.Utf8());
  if (!stats || !IsWhitelistedStats(*stats))
    return std::unique_ptr<blink::WebRTCStats>();
  return std::unique_ptr<blink::WebRTCStats>(
      new RTCStats(stats_report_, stats));
}

std::unique_ptr<blink::WebRTCStats> RTCStatsReport::Next() {
  while (it_ != end_) {
    const webrtc::RTCStats& next = *it_;
    ++it_;
    if (IsWhitelistedStats(next)) {
      return std::unique_ptr<blink::WebRTCStats>(
          new RTCStats(stats_report_, &next));
    }
  }
  return std::unique_ptr<blink::WebRTCStats>();
}

size_t RTCStatsReport::Size() const {
  return stats_report_->size();
}

RTCStats::RTCStats(
    const scoped_refptr<const webrtc::RTCStatsReport>& stats_owner,
    const webrtc::RTCStats* stats)
    : stats_owner_(stats_owner),
      stats_(stats),
      stats_members_(stats->Members()) {
  DCHECK(stats_owner_);
  DCHECK(stats_);
  DCHECK(stats_owner_->Get(stats_->id()));
}

RTCStats::~RTCStats() {
}

blink::WebString RTCStats::Id() const {
  return blink::WebString::FromUTF8(stats_->id());
}

blink::WebString RTCStats::GetType() const {
  return blink::WebString::FromUTF8(stats_->type());
}

double RTCStats::Timestamp() const {
  return stats_->timestamp_us() / static_cast<double>(
      base::Time::kMicrosecondsPerMillisecond);
}

size_t RTCStats::MembersCount() const {
  return stats_members_.size();
}

std::unique_ptr<blink::WebRTCStatsMember> RTCStats::GetMember(size_t i) const {
  DCHECK_LT(i, stats_members_.size());
  return std::unique_ptr<blink::WebRTCStatsMember>(
      new RTCStatsMember(stats_owner_, stats_members_[i]));
}

RTCStatsMember::RTCStatsMember(
    const scoped_refptr<const webrtc::RTCStatsReport>& stats_owner,
    const webrtc::RTCStatsMemberInterface* member)
    : stats_owner_(stats_owner),
      member_(member) {
  DCHECK(stats_owner_);
  DCHECK(member_);
}

RTCStatsMember::~RTCStatsMember() {
}

blink::WebString RTCStatsMember::GetName() const {
  return blink::WebString::FromUTF8(member_->name());
}

blink::WebRTCStatsMemberType RTCStatsMember::GetType() const {
  switch (member_->type()) {
    case webrtc::RTCStatsMemberInterface::kBool:
      return blink::kWebRTCStatsMemberTypeBool;
    case webrtc::RTCStatsMemberInterface::kInt32:
      return blink::kWebRTCStatsMemberTypeInt32;
    case webrtc::RTCStatsMemberInterface::kUint32:
      return blink::kWebRTCStatsMemberTypeUint32;
    case webrtc::RTCStatsMemberInterface::kInt64:
      return blink::kWebRTCStatsMemberTypeInt64;
    case webrtc::RTCStatsMemberInterface::kUint64:
      return blink::kWebRTCStatsMemberTypeUint64;
    case webrtc::RTCStatsMemberInterface::kDouble:
      return blink::kWebRTCStatsMemberTypeDouble;
    case webrtc::RTCStatsMemberInterface::kString:
      return blink::kWebRTCStatsMemberTypeString;
    case webrtc::RTCStatsMemberInterface::kSequenceBool:
      return blink::kWebRTCStatsMemberTypeSequenceBool;
    case webrtc::RTCStatsMemberInterface::kSequenceInt32:
      return blink::kWebRTCStatsMemberTypeSequenceInt32;
    case webrtc::RTCStatsMemberInterface::kSequenceUint32:
      return blink::kWebRTCStatsMemberTypeSequenceUint32;
    case webrtc::RTCStatsMemberInterface::kSequenceInt64:
      return blink::kWebRTCStatsMemberTypeSequenceInt64;
    case webrtc::RTCStatsMemberInterface::kSequenceUint64:
      return blink::kWebRTCStatsMemberTypeSequenceUint64;
    case webrtc::RTCStatsMemberInterface::kSequenceDouble:
      return blink::kWebRTCStatsMemberTypeSequenceDouble;
    case webrtc::RTCStatsMemberInterface::kSequenceString:
      return blink::kWebRTCStatsMemberTypeSequenceString;
    default:
      NOTREACHED();
      return blink::kWebRTCStatsMemberTypeSequenceInt32;
  }
}

bool RTCStatsMember::IsDefined() const {
  return member_->is_defined();
}

bool RTCStatsMember::ValueBool() const {
  DCHECK(IsDefined());
  return *member_->cast_to<webrtc::RTCStatsMember<bool>>();
}

int32_t RTCStatsMember::ValueInt32() const {
  DCHECK(IsDefined());
  return *member_->cast_to<webrtc::RTCStatsMember<int32_t>>();
}

uint32_t RTCStatsMember::ValueUint32() const {
  DCHECK(IsDefined());
  return *member_->cast_to<webrtc::RTCStatsMember<uint32_t>>();
}

int64_t RTCStatsMember::ValueInt64() const {
  DCHECK(IsDefined());
  return *member_->cast_to<webrtc::RTCStatsMember<int64_t>>();
}

uint64_t RTCStatsMember::ValueUint64() const {
  DCHECK(IsDefined());
  return *member_->cast_to<webrtc::RTCStatsMember<uint64_t>>();
}

double RTCStatsMember::ValueDouble() const {
  DCHECK(IsDefined());
  return *member_->cast_to<webrtc::RTCStatsMember<double>>();
}

blink::WebString RTCStatsMember::ValueString() const {
  DCHECK(IsDefined());
  return blink::WebString::FromUTF8(
      *member_->cast_to<webrtc::RTCStatsMember<std::string>>());
}

blink::WebVector<int> RTCStatsMember::ValueSequenceBool() const {
  DCHECK(IsDefined());
  const std::vector<bool>& vector =
      *member_->cast_to<webrtc::RTCStatsMember<std::vector<bool>>>();
  std::vector<int> uint32_vector;
  uint32_vector.reserve(vector.size());
  for (size_t i = 0; i < vector.size(); ++i) {
    uint32_vector.push_back(vector[i] ? 1 : 0);
  }
  return blink::WebVector<int>(uint32_vector);
}

blink::WebVector<int32_t> RTCStatsMember::ValueSequenceInt32() const {
  DCHECK(IsDefined());
  return blink::WebVector<int32_t>(
      *member_->cast_to<webrtc::RTCStatsMember<std::vector<int32_t>>>());
}

blink::WebVector<uint32_t> RTCStatsMember::ValueSequenceUint32() const {
  DCHECK(IsDefined());
  return blink::WebVector<uint32_t>(
      *member_->cast_to<webrtc::RTCStatsMember<std::vector<uint32_t>>>());
}

blink::WebVector<int64_t> RTCStatsMember::ValueSequenceInt64() const {
  DCHECK(IsDefined());
  return blink::WebVector<int64_t>(
      *member_->cast_to<webrtc::RTCStatsMember<std::vector<int64_t>>>());
}

blink::WebVector<uint64_t> RTCStatsMember::ValueSequenceUint64() const {
  DCHECK(IsDefined());
  return blink::WebVector<uint64_t>(
      *member_->cast_to<webrtc::RTCStatsMember<std::vector<uint64_t>>>());
}

blink::WebVector<double> RTCStatsMember::ValueSequenceDouble() const {
  DCHECK(IsDefined());
  return blink::WebVector<double>(
      *member_->cast_to<webrtc::RTCStatsMember<std::vector<double>>>());
}

blink::WebVector<blink::WebString> RTCStatsMember::ValueSequenceString() const {
  DCHECK(IsDefined());
  const std::vector<std::string>& sequence =
      *member_->cast_to<webrtc::RTCStatsMember<std::vector<std::string>>>();
  blink::WebVector<blink::WebString> web_sequence(sequence.size());
  for (size_t i = 0; i < sequence.size(); ++i)
    web_sequence[i] = blink::WebString::FromUTF8(sequence[i]);
  return web_sequence;
}

// static
rtc::scoped_refptr<RTCStatsCollectorCallbackImpl>
RTCStatsCollectorCallbackImpl::Create(
    scoped_refptr<base::SingleThreadTaskRunner> main_thread,
    std::unique_ptr<blink::WebRTCStatsReportCallback> callback) {
  return rtc::scoped_refptr<RTCStatsCollectorCallbackImpl>(
      new rtc::RefCountedObject<RTCStatsCollectorCallbackImpl>(
          std::move(main_thread), callback.release()));
}

RTCStatsCollectorCallbackImpl::RTCStatsCollectorCallbackImpl(
    scoped_refptr<base::SingleThreadTaskRunner> main_thread,
    blink::WebRTCStatsReportCallback* callback)
    : main_thread_(std::move(main_thread)), callback_(callback) {}

RTCStatsCollectorCallbackImpl::~RTCStatsCollectorCallbackImpl() {
  DCHECK(!callback_);
}

void RTCStatsCollectorCallbackImpl::OnStatsDelivered(
    const rtc::scoped_refptr<const webrtc::RTCStatsReport>& report) {
  main_thread_->PostTask(
      FROM_HERE,
      base::BindOnce(
          &RTCStatsCollectorCallbackImpl::OnStatsDeliveredOnMainThread, this,
          report));
}

void RTCStatsCollectorCallbackImpl::OnStatsDeliveredOnMainThread(
    rtc::scoped_refptr<const webrtc::RTCStatsReport> report) {
  DCHECK(main_thread_->BelongsToCurrentThread());
  DCHECK(report);
  DCHECK(callback_);
  callback_->OnStatsDelivered(std::unique_ptr<blink::WebRTCStatsReport>(
      new RTCStatsReport(base::WrapRefCounted(report.get()))));
  // Make sure the callback is destroyed in the main thread as well.
  callback_.reset();
}

void WhitelistStatsForTesting(const char* type) {
  GetStatsWhitelist()->WhitelistStatsForTesting(type);
}

}  // namespace content
