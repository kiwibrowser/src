/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "modules/congestion_controller/rtp/send_time_history.h"

#include <algorithm>
#include <utility>

#include "modules/rtp_rtcp/include/rtp_rtcp_defines.h"
#include "rtc_base/checks.h"
#include "system_wrappers/include/clock.h"

namespace webrtc {
namespace webrtc_cc {

SendTimeHistory::SendTimeHistory(const Clock* clock,
                                 int64_t packet_age_limit_ms)
    : clock_(clock), packet_age_limit_ms_(packet_age_limit_ms) {}

SendTimeHistory::~SendTimeHistory() {}

void SendTimeHistory::AddAndRemoveOld(const PacketFeedback& packet) {
  int64_t now_ms = clock_->TimeInMilliseconds();
  // Remove old.
  while (!history_.empty() &&
         now_ms - history_.begin()->second.creation_time_ms >
             packet_age_limit_ms_) {
    // TODO(sprang): Warn if erasing (too many) old items?
    history_.erase(history_.begin());
  }

  // Add new.
  int64_t unwrapped_seq_num = seq_num_unwrapper_.Unwrap(packet.sequence_number);
  PacketFeedback packet_copy = packet;
  packet_copy.long_sequence_number = unwrapped_seq_num;
  history_.insert(std::make_pair(unwrapped_seq_num, packet_copy));
}

bool SendTimeHistory::OnSentPacket(uint16_t sequence_number,
                                   int64_t send_time_ms) {
  int64_t unwrapped_seq_num = seq_num_unwrapper_.Unwrap(sequence_number);
  auto it = history_.find(unwrapped_seq_num);
  if (it == history_.end())
    return false;
  it->second.send_time_ms = send_time_ms;
  return true;
}

rtc::Optional<PacketFeedback> SendTimeHistory::GetPacket(
    uint16_t sequence_number) const {
  int64_t unwrapped_seq_num =
      seq_num_unwrapper_.UnwrapWithoutUpdate(sequence_number);
  rtc::Optional<PacketFeedback> optional_feedback;
  auto it = history_.find(unwrapped_seq_num);
  if (it != history_.end())
    optional_feedback.emplace(it->second);
  return optional_feedback;
}

bool SendTimeHistory::GetFeedback(PacketFeedback* packet_feedback,
                                  bool remove) {
  RTC_DCHECK(packet_feedback);
  int64_t unwrapped_seq_num =
      seq_num_unwrapper_.Unwrap(packet_feedback->sequence_number);
  latest_acked_seq_num_.emplace(
      std::max(unwrapped_seq_num, latest_acked_seq_num_.value_or(0)));
  RTC_DCHECK_GE(*latest_acked_seq_num_, 0);
  auto it = history_.find(unwrapped_seq_num);
  if (it == history_.end())
    return false;

  // Save arrival_time not to overwrite it.
  int64_t arrival_time_ms = packet_feedback->arrival_time_ms;
  *packet_feedback = it->second;
  packet_feedback->arrival_time_ms = arrival_time_ms;

  if (remove)
    history_.erase(it);
  return true;
}

size_t SendTimeHistory::GetOutstandingBytes(uint16_t local_net_id,
                                            uint16_t remote_net_id) const {
  size_t outstanding_bytes = 0;
  auto unacked_it = history_.begin();
  if (latest_acked_seq_num_) {
    unacked_it = history_.lower_bound(*latest_acked_seq_num_);
  }
  for (; unacked_it != history_.end(); ++unacked_it) {
    if (unacked_it->second.local_net_id == local_net_id &&
        unacked_it->second.remote_net_id == remote_net_id &&
        unacked_it->second.send_time_ms >= 0) {
      outstanding_bytes += unacked_it->second.payload_size;
    }
  }
  return outstanding_bytes;
}

}  // namespace webrtc_cc
}  // namespace webrtc
