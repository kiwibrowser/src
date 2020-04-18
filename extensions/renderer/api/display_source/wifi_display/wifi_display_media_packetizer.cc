// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/api/display_source/wifi_display/wifi_display_media_packetizer.h"

#include <utility>

#include "base/logging.h"
#include "base/rand_util.h"
#include "crypto/random.h"
#include "extensions/renderer/api/display_source/wifi_display/wifi_display_elementary_stream_info.h"

namespace extensions {
namespace {
const size_t kMaxTransportStreamPacketCount = 7u;
const uint8_t kProtocolPayloadTypeMP2T = 33u;
const uint8_t kProtocolVersion = 2u;
}  // namespace

WiFiDisplayMediaDatagramPacket::WiFiDisplayMediaDatagramPacket() = default;

WiFiDisplayMediaDatagramPacket::WiFiDisplayMediaDatagramPacket(
    WiFiDisplayMediaDatagramPacket&&) = default;

WiFiDisplayMediaPacketizer::WiFiDisplayMediaPacketizer(
    const base::TimeDelta& delay_for_unit_time_stamps,
    const std::vector<WiFiDisplayElementaryStreamInfo>& stream_infos,
    const PacketizedCallback& on_packetized)
    : WiFiDisplayTransportStreamPacketizer(delay_for_unit_time_stamps,
                                           stream_infos),
      on_packetized_media_datagram_packet_(on_packetized) {
  // Sequence numbers are mainly used for detecting lossed packets within one
  // RTP session. The initial value SHOULD be random (unpredictable) to make
  // known-plaintext attacks on encryption more difficult, in case the packets
  // flow through a translator that encrypts them.
  crypto::RandBytes(&sequence_number_, sizeof(sequence_number_));
  base::RandBytes(&synchronization_source_identifier_,
                  sizeof(synchronization_source_identifier_));
}

WiFiDisplayMediaPacketizer::~WiFiDisplayMediaPacketizer() {}

bool WiFiDisplayMediaPacketizer::OnPacketizedTransportStreamPacket(
    const WiFiDisplayTransportStreamPacket& transport_stream_packet,
    bool flush) {
  DCHECK(CalledOnValidThread());

  if (media_datagram_packet_.empty()) {
    // Convert time to the number of 90 kHz ticks since some epoch.
    const uint64_t us = static_cast<uint64_t>(
        (base::TimeTicks::Now() - base::TimeTicks()).InMicroseconds());
    const uint64_t time_stamp = (us * 90u + 500u) / 1000u;
    const uint8_t header_without_identifiers[] = {
        (kProtocolVersion << 6 |          // Version (2 bits)
         0x0u << 5 |                      // Padding (no)
         0x0u << 4 |                      // Extension (no)
         0u << 0),                        // CSRC count (4 bits)
        (0x0u << 7 |                      // Marker (no)
         kProtocolPayloadTypeMP2T << 0),  // Payload type (7 bits)
        sequence_number_ >> 8,            // Sequence number (16 bits)
        sequence_number_ & 0xFFu,         //
        (time_stamp >> 24) & 0xFFu,       // Time stamp (32 bits)
        (time_stamp >> 16) & 0xFFu,       //
        (time_stamp >> 8) & 0xFFu,        //
        time_stamp & 0xFFu};              //
    ++sequence_number_;
    media_datagram_packet_.reserve(
        sizeof(header_without_identifiers) +
        sizeof(synchronization_source_identifier_) +
        kMaxTransportStreamPacketCount *
            WiFiDisplayTransportStreamPacket::kPacketSize);
    media_datagram_packet_.insert(media_datagram_packet_.end(),
                                  std::begin(header_without_identifiers),
                                  std::end(header_without_identifiers));
    media_datagram_packet_.insert(
        media_datagram_packet_.end(),
        std::begin(synchronization_source_identifier_),
        std::end(synchronization_source_identifier_));
    DCHECK_EQ(0u, media_datagram_packet_.size() /
                      WiFiDisplayTransportStreamPacket::kPacketSize);
  }

  // Append header and payload data.
  media_datagram_packet_.insert(media_datagram_packet_.end(),
                                transport_stream_packet.header().begin(),
                                transport_stream_packet.header().end());
  media_datagram_packet_.insert(media_datagram_packet_.end(),
                                transport_stream_packet.payload().begin(),
                                transport_stream_packet.payload().end());
  media_datagram_packet_.insert(media_datagram_packet_.end(),
                                transport_stream_packet.filler().size(),
                                transport_stream_packet.filler().value());

  // Combine multiple transport stream packets into one datagram packet
  // by delaying delegation whenever possible.
  if (!flush) {
    const size_t transport_stream_packet_count =
        media_datagram_packet_.size() /
        WiFiDisplayTransportStreamPacket::kPacketSize;
    if (transport_stream_packet_count < kMaxTransportStreamPacketCount)
      return true;
  }

  WiFiDisplayMediaDatagramPacket packet;
  packet.swap(media_datagram_packet_);
  return on_packetized_media_datagram_packet_.Run(std::move(packet));
}

}  // namespace extensions
