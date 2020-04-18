// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_RENDERER_API_DISPLAY_SOURCE_WIFI_DISPLAY_WIFI_DISPLAY_ELEMENTARY_STREAM_PACKETIZER_H_
#define EXTENSIONS_RENDERER_API_DISPLAY_SOURCE_WIFI_DISPLAY_WIFI_DISPLAY_ELEMENTARY_STREAM_PACKETIZER_H_

#include "base/time/time.h"
#include "extensions/renderer/api/display_source/wifi_display/wifi_display_stream_packet_part.h"

namespace extensions {

// WiFi Display elementary stream packet represents a Packetized Elementary
// Stream (PES) packet containing WiFi Display elementary stream unit data.
class WiFiDisplayElementaryStreamPacket {
 public:
  using HeaderBuffer = uint8_t[22];

  WiFiDisplayElementaryStreamPacket(const HeaderBuffer& header_data,
                                    size_t header_size,
                                    const uint8_t* unit_header_data,
                                    size_t unit_header_size,
                                    const uint8_t* unit_data,
                                    size_t unit_size);
  // WiFiDisplayElementaryStreamPacketizer::EncodeElementaryStreamUnit returns
  // WiFiDisplayElementaryStreamPacket so WiFiDisplayElementaryStreamPacket
  // must be move constructible (as it is not copy constructible).
  // A compiler should however use return value optimization and elide each
  // call to this move constructor.
  WiFiDisplayElementaryStreamPacket(WiFiDisplayElementaryStreamPacket&& other);

  const WiFiDisplayStreamPacketPart& header() const { return header_; }
  const WiFiDisplayStreamPacketPart& unit_header() const {
    return unit_header_;
  }
  const WiFiDisplayStreamPacketPart& unit() const { return unit_; }

 private:
  HeaderBuffer header_buffer_;
  WiFiDisplayStreamPacketPart header_;
  WiFiDisplayStreamPacketPart unit_header_;
  WiFiDisplayStreamPacketPart unit_;

  DISALLOW_COPY_AND_ASSIGN(WiFiDisplayElementaryStreamPacket);
};

// The WiFi Display elementary stream packetizer packetizes unit buffers to
// Packetized Elementary Stream (PES) packets.
// It is used internally by a WiFi Display transport stream packetizer.
class WiFiDisplayElementaryStreamPacketizer {
 public:
  enum : uint8_t {
    kPrivateStream1Id = 0xBDu,
    kFirstAudioStreamId = 0xC0u,
    kLastAudioStreamId = 0xDFu,
    kFirstVideoStreamId = 0xE0u,
    kLastVideoStreamId = 0xEFu,
  };

  static WiFiDisplayElementaryStreamPacket EncodeElementaryStreamUnit(
      uint8_t stream_id,
      const uint8_t* unit_header_data,
      size_t unit_header_size,
      const uint8_t* unit_data,
      size_t unit_size,
      const base::TimeTicks& pts,
      const base::TimeTicks& dts);
};

}  // namespace extensions

#endif  // EXTENSIONS_RENDERER_API_DISPLAY_SOURCE_WIFI_DISPLAY_WIFI_DISPLAY_ELEMENTARY_STREAM_PACKETIZER_H_
