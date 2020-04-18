/*
 *  Copyright (c) 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "logging/rtc_event_log/icelogger.h"

#include "logging/rtc_event_log/rtc_event_log.h"
#include "rtc_base/ptr_util.h"

namespace webrtc {

IceEventLog::IceEventLog() {}
IceEventLog::~IceEventLog() {}

bool IceEventLog::IsIceCandidatePairConfigEvent(
    IceCandidatePairEventType type) {
  return (type == IceCandidatePairEventType::kAdded) ||
         (type == IceCandidatePairEventType::kUpdated) ||
         (type == IceCandidatePairEventType::kDestroyed) ||
         (type == IceCandidatePairEventType::kSelected);
}

void IceEventLog::LogCandidatePairEvent(
    IceCandidatePairEventType type,
    uint32_t candidate_pair_id,
    const IceCandidatePairDescription& candidate_pair_desc) {
  if (event_log_ == nullptr) {
    return;
  }
  if (IsIceCandidatePairConfigEvent(type)) {
    candidate_pair_desc_by_id_[candidate_pair_id] = candidate_pair_desc;
    event_log_->Log(rtc::MakeUnique<RtcEventIceCandidatePairConfig>(
        type, candidate_pair_id, candidate_pair_desc));
    return;
  }
  event_log_->Log(
      rtc::MakeUnique<RtcEventIceCandidatePair>(type, candidate_pair_id));
}

void IceEventLog::DumpCandidatePairDescriptionToMemoryAsConfigEvents() const {
  for (const auto& desc_id_pair : candidate_pair_desc_by_id_) {
    event_log_->Log(rtc::MakeUnique<RtcEventIceCandidatePairConfig>(
        IceCandidatePairEventType::kUpdated, desc_id_pair.first,
        desc_id_pair.second));
  }
}

}  // namespace webrtc
