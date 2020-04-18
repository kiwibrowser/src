/*
 *  Copyright (c) 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef CALL_RTP_TRANSPORT_CONTROLLER_SEND_INTERFACE_H_
#define CALL_RTP_TRANSPORT_CONTROLLER_SEND_INTERFACE_H_
#include <stddef.h>
#include <stdint.h>

#include <string>

#include "api/optional.h"
#include "api/transport/bitrate_settings.h"
#include "call/bitrate_constraints.h"

namespace rtc {
struct SentPacket;
struct NetworkRoute;
class TaskQueue;
}  // namespace rtc
namespace webrtc {

class CallStatsObserver;
class TargetTransferRateObserver;
class Module;
class PacedSender;
class PacketFeedbackObserver;
class PacketRouter;
class RateLimiter;
class RtcpBandwidthObserver;
class RtpPacketSender;
struct RtpKeepAliveConfig;
class TransportFeedbackObserver;

// An RtpTransportController should own everything related to the RTP
// transport to/from a remote endpoint. We should have separate
// interfaces for send and receive side, even if they are implemented
// by the same class. This is an ongoing refactoring project. At some
// point, this class should be promoted to a public api under
// webrtc/api/rtp/.
//
// For a start, this object is just a collection of the objects needed
// by the VideoSendStream constructor. The plan is to move ownership
// of all RTP-related objects here, and add methods to create per-ssrc
// objects which would then be passed to VideoSendStream. Eventually,
// direct accessors like packet_router() should be removed.
//
// This should also have a reference to the underlying
// webrtc::Transport(s). Currently, webrtc::Transport is implemented by
// WebRtcVideoChannel and WebRtcVoiceMediaChannel, and owned by
// WebrtcSession. Video and audio always uses different transport
// objects, even in the common case where they are bundled over the
// same underlying transport.
//
// Extracting the logic of the webrtc::Transport from BaseChannel and
// subclasses into a separate class seems to be a prerequesite for
// moving the transport here.
class RtpTransportControllerSendInterface {
 public:
  virtual ~RtpTransportControllerSendInterface() {}
  virtual rtc::TaskQueue* GetWorkerQueue() = 0;
  virtual PacketRouter* packet_router() = 0;
  virtual TransportFeedbackObserver* transport_feedback_observer() = 0;

  virtual RtpPacketSender* packet_sender() = 0;
  virtual const RtpKeepAliveConfig& keepalive_config() const = 0;

  // SetAllocatedSendBitrateLimits sets bitrates limits imposed by send codec
  // settings.
  // |min_send_bitrate_bps| is the total minimum send bitrate required by all
  // sending streams.  This is the minimum bitrate the PacedSender will use.
  // Note that SendSideCongestionController::OnNetworkChanged can still be
  // called with a lower bitrate estimate. |max_padding_bitrate_bps| is the max
  // bitrate the send streams request for padding. This can be higher than the
  // current network estimate and tells the PacedSender how much it should max
  // pad unless there is real packets to send.
  virtual void SetAllocatedSendBitrateLimits(int min_send_bitrate_bps,
                                             int max_padding_bitrate_bps,
                                             int total_bitrate_bps) = 0;

  virtual void SetPacingFactor(float pacing_factor) = 0;
  virtual void SetQueueTimeLimit(int limit_ms) = 0;

  virtual CallStatsObserver* GetCallStatsObserver() = 0;

  virtual void RegisterPacketFeedbackObserver(
      PacketFeedbackObserver* observer) = 0;
  virtual void DeRegisterPacketFeedbackObserver(
      PacketFeedbackObserver* observer) = 0;
  virtual void RegisterTargetTransferRateObserver(
      TargetTransferRateObserver* observer) = 0;
  virtual void OnNetworkRouteChanged(
      const std::string& transport_name,
      const rtc::NetworkRoute& network_route) = 0;
  virtual void OnNetworkAvailability(bool network_available) = 0;
  virtual RtcpBandwidthObserver* GetBandwidthObserver() = 0;
  virtual int64_t GetPacerQueuingDelayMs() const = 0;
  virtual int64_t GetFirstPacketTimeMs() const = 0;
  virtual void EnablePeriodicAlrProbing(bool enable) = 0;
  virtual void OnSentPacket(const rtc::SentPacket& sent_packet) = 0;
  virtual void SetPerPacketFeedbackAvailable(bool available) = 0;

  virtual void SetSdpBitrateParameters(
      const BitrateConstraints& constraints) = 0;
  virtual void SetClientBitratePreferences(
      const BitrateSettings& preferences) = 0;
};

}  // namespace webrtc

#endif  // CALL_RTP_TRANSPORT_CONTROLLER_SEND_INTERFACE_H_
