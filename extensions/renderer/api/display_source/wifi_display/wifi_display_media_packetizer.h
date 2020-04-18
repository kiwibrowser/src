// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_RENDERER_API_DISPLAY_SOURCE_WIFI_DISPLAY_WIFI_DISPLAY_MEDIA_PACKETIZER_H_
#define EXTENSIONS_RENDERER_API_DISPLAY_SOURCE_WIFI_DISPLAY_WIFI_DISPLAY_MEDIA_PACKETIZER_H_

#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "extensions/renderer/api/display_source/wifi_display/wifi_display_transport_stream_packetizer.h"

namespace extensions {

// This class represents an RTP datagram packet containing MPEG Transport
// Stream (MPEG-TS) packets as a payload.
class WiFiDisplayMediaDatagramPacket : public std::vector<uint8_t> {
 public:
  WiFiDisplayMediaDatagramPacket();
  WiFiDisplayMediaDatagramPacket(WiFiDisplayMediaDatagramPacket&&);

 private:
  DISALLOW_COPY_AND_ASSIGN(WiFiDisplayMediaDatagramPacket);
};

// The WiFi Display media packetizer packetizes unit buffers to media datagram
// packets containing MPEG Transport Stream (MPEG-TS) packets containing either
// meta information or Packetized Elementary Stream (PES) packets  containing
// unit data.
//
// Whenever a media datagram packet is fully created and thus ready for further
// processing, a callback is called.
class WiFiDisplayMediaPacketizer : public WiFiDisplayTransportStreamPacketizer {
 public:
  using PacketizedCallback =
      base::Callback<bool(WiFiDisplayMediaDatagramPacket)>;

  WiFiDisplayMediaPacketizer(
      const base::TimeDelta& delay_for_unit_time_stamps,
      const std::vector<WiFiDisplayElementaryStreamInfo>& stream_infos,
      const PacketizedCallback& on_packetized);
  ~WiFiDisplayMediaPacketizer() override;

 protected:
  bool OnPacketizedTransportStreamPacket(
      const WiFiDisplayTransportStreamPacket& transport_stream_packet,
      bool flush) override;

 private:
  using SourceIdentifier = uint8_t[4];

  WiFiDisplayMediaDatagramPacket media_datagram_packet_;
  PacketizedCallback on_packetized_media_datagram_packet_;
  uint16_t sequence_number_;
  SourceIdentifier synchronization_source_identifier_;
};

}  // namespace extensions

#endif  // EXTENSIONS_RENDERER_API_DISPLAY_SOURCE_WIFI_DISPLAY_WIFI_DISPLAY_MEDIA_PACKETIZER_H_
