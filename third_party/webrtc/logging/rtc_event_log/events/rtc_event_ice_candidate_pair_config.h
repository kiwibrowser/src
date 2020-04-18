/*
 *  Copyright (c) 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef LOGGING_RTC_EVENT_LOG_EVENTS_RTC_EVENT_ICE_CANDIDATE_PAIR_CONFIG_H_
#define LOGGING_RTC_EVENT_LOG_EVENTS_RTC_EVENT_ICE_CANDIDATE_PAIR_CONFIG_H_

#include "logging/rtc_event_log/events/rtc_event.h"

#include "logging/rtc_event_log/events/rtc_event_ice_candidate_pair.h"

#include <string>

namespace webrtc {

// TODO(qingsi): Change the names of candidate types to "host", "srflx", "prflx"
// and "relay" after the naming is spec-compliant in the signaling part
enum class IceCandidateType {
  kLocal,
  kStun,
  kPrflx,
  kRelay,
  kUnknown,
};

enum class IceCandidatePairProtocol {
  kUdp,
  kTcp,
  kSsltcp,
  kTls,
  kUnknown,
};

enum class IceCandidatePairAddressFamily {
  kIpv4,
  kIpv6,
  kUnknown,
};

enum class IceCandidateNetworkType {
  kEthernet,
  kLoopback,
  kWifi,
  kVpn,
  kCellular,
  kUnknown,
};

class IceCandidatePairDescription {
 public:
  IceCandidatePairDescription();
  explicit IceCandidatePairDescription(
      const IceCandidatePairDescription& other);

  ~IceCandidatePairDescription();

  IceCandidateType local_candidate_type;
  IceCandidatePairProtocol local_relay_protocol;
  IceCandidateNetworkType local_network_type;
  IceCandidatePairAddressFamily local_address_family;
  IceCandidateType remote_candidate_type;
  IceCandidatePairAddressFamily remote_address_family;
  IceCandidatePairProtocol candidate_pair_protocol;
};

class RtcEventIceCandidatePairConfig final : public RtcEvent {
 public:
  RtcEventIceCandidatePairConfig(
      IceCandidatePairEventType type,
      uint32_t candidate_pair_id,
      const IceCandidatePairDescription& candidate_pair_desc);

  ~RtcEventIceCandidatePairConfig() override;

  Type GetType() const override;

  bool IsConfigEvent() const override;

  const IceCandidatePairEventType type_;
  const uint32_t candidate_pair_id_;
  const IceCandidatePairDescription candidate_pair_desc_;
};

}  // namespace webrtc

#endif  // LOGGING_RTC_EVENT_LOG_EVENTS_RTC_EVENT_ICE_CANDIDATE_PAIR_CONFIG_H_
