// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/api/display_source/wifi_display/wifi_display_elementary_stream_packetizer.h"

#include <cstring>

#include "base/logging.h"

namespace extensions {
namespace {

// Code and parameters related to the Packetized Elementary Stream (PES)
// specification.
namespace pes {

const size_t kUnitDataAlignment = 4u;

const size_t kOptionalHeaderBaseSize = 3u;
const size_t kPacketHeaderBaseSize = 6u;
const size_t kStuffingBytesMaxSize = kUnitDataAlignment - 1u;
const size_t kTimeStampSize = 5u;
const size_t kPacketHeaderMaxSize =
    kPacketHeaderBaseSize + kOptionalHeaderBaseSize + 2u * kTimeStampSize +
    kStuffingBytesMaxSize;

size_t FillInTimeStamp(uint8_t* dst,
                       uint8_t pts_dts_indicator,
                       const base::TimeTicks& ts) {
  // Convert to the number of 90 kHz ticks since some epoch.
  // Always round up so that the number of ticks is never smaller than
  // the number of program clock reference base ticks (which is not rounded
  // because program clock reference is encoded with higher precision).
  const uint64_t us =
      static_cast<uint64_t>((ts - base::TimeTicks()).InMicroseconds());
  const uint64_t n = (us * 90u + 999u) / 1000u;

  // Expand PTS DTS indicator and a 33 bit time stamp to 40 bits:
  //  * 4 PTS DTS indicator bits, 3 time stamp bits, 1 on bit
  //  * 15 time stamp bits, 1 on bit
  //  * 15 time stamp bits, 1 on bit
  size_t i = 0u;
  dst[i++] = (pts_dts_indicator << 4) | (((n >> 30) & 0x7u) << 1) | (0x1u << 0);
  dst[i++] = (n >> 22) & 0xFFu;
  dst[i++] = (((n >> 15) & 0x7Fu) << 1) | (0x1u << 0);
  dst[i++] = (n >> 7) & 0xFFu;
  dst[i++] = (((n >> 0) & 0x7Fu) << 1) | (0x1u << 0);
  DCHECK_EQ(i, kTimeStampSize);
  return i;
}

size_t FillInOptionalHeader(uint8_t* dst,
                            const base::TimeTicks& pts,
                            const base::TimeTicks& dts,
                            size_t unit_header_size) {
  size_t i = 0u;
  dst[i++] = (0x2u << 6) |  // Marker bits (0b10)
             (0x0u << 4) |  // Scrambling control (0b00 for not)
             (0x0u << 3) |  // Priority
             (0x0u << 2) |  // Data alignment indicator (0b0 for not)
             (0x0u << 1) |  // Copyright (0b0 for not)
             (0x0u << 0);   // Original (0b0 for copy)
  const uint8_t pts_dts_indicator =
      !pts.is_null() ? (!dts.is_null() ? 0x3u : 0x2u) : 0x0u;
  dst[i++] = (pts_dts_indicator << 6) |  // PTS DTS indicator
             (0x0u << 5) |               // ESCR flag
             (0x0u << 4) |               // ES rate flag
             (0x0u << 3) |               // DSM trick mode flag
             (0x0u << 2) |               // Additional copy info flag
             (0x0u << 1) |               // CRC flag
             (0x0u << 0);                // Extension flag
  const size_t header_length_index = i++;
  const size_t optional_header_base_end_index = i;
  DCHECK_EQ(i, kOptionalHeaderBaseSize);

  // Optional fields:
  // PTS and DTS.
  if (!pts.is_null()) {
    i += FillInTimeStamp(&dst[i], pts_dts_indicator, pts);
    if (!dts.is_null())
      i += FillInTimeStamp(&dst[i], 0x1u, dts);
  }

  // Stuffing bytes (for unit data alignment).
  const size_t remainder =
      (kPacketHeaderBaseSize + i + unit_header_size) % kUnitDataAlignment;
  if (remainder) {
    const size_t n = kUnitDataAlignment - remainder;
    DCHECK_LE(n, kStuffingBytesMaxSize);
    std::memset(&dst[i], 0xFF, n);
    i += n;
  }

  dst[header_length_index] = i - optional_header_base_end_index;
  return i;
}

size_t FillInPacketHeader(uint8_t* dst,
                          uint8_t stream_id,
                          const base::TimeTicks& pts,
                          const base::TimeTicks& dts,
                          size_t unit_header_size,
                          size_t unit_size) {
  // Reserve space for packet header base.
  size_t i = kPacketHeaderBaseSize;
  const size_t header_base_end_index = i;

  // Fill in optional header.
  i += FillInOptionalHeader(&dst[i], pts, dts, unit_header_size);

  // Compute packet length.
  size_t packet_length =
      (i - header_base_end_index) + unit_header_size + unit_size;
  if (packet_length >> 16) {
    // The packet length is too large to be represented. That should only
    // happen for video frames for which the packet length is not mandatory
    // but may be set to 0, too.
    DCHECK_GE(static_cast<unsigned>(stream_id),
              WiFiDisplayElementaryStreamPacketizer::kFirstVideoStreamId);
    DCHECK_LE(static_cast<unsigned>(stream_id),
              WiFiDisplayElementaryStreamPacketizer::kLastVideoStreamId);
    packet_length = 0u;
  }

  // Fill in packet header base.
  size_t j = 0u;
  dst[j++] = 0x00u;  // Packet start code prefix (0x000001 in three bytes).
  dst[j++] = 0x00u;  //
  dst[j++] = 0x01u;  //
  dst[j++] = stream_id;
  dst[j++] = packet_length >> 8;
  dst[j++] = packet_length & 0xFFu;
  DCHECK_EQ(j, kPacketHeaderBaseSize);

  return i;
}

}  // namespace pes

}  // namespace

WiFiDisplayElementaryStreamPacket::WiFiDisplayElementaryStreamPacket(
    const HeaderBuffer& header_data,
    size_t header_size,
    const uint8_t* unit_header_data,
    size_t unit_header_size,
    const uint8_t* unit_data,
    size_t unit_size)
    : header_(header_buffer_, header_size),
      unit_header_(unit_header_data, unit_header_size),
      unit_(unit_data, unit_size) {
  // Copy the actual header data bytes from the |header_data| argument to
  // the |header_buffer_| member buffer used in the member initialization list.
  std::memcpy(header_buffer_, header_data, header_.size());
}

WiFiDisplayElementaryStreamPacket::WiFiDisplayElementaryStreamPacket(
    WiFiDisplayElementaryStreamPacket&& other)
    : WiFiDisplayElementaryStreamPacket(other.header_buffer_,
                                        other.header_.size(),
                                        other.unit_header_.data(),
                                        other.unit_header_.size(),
                                        other.unit_.data(),
                                        other.unit_.size()) {}

// static
WiFiDisplayElementaryStreamPacket
WiFiDisplayElementaryStreamPacketizer::EncodeElementaryStreamUnit(
    uint8_t stream_id,
    const uint8_t* unit_header_data,
    size_t unit_header_size,
    const uint8_t* unit_data,
    size_t unit_size,
    const base::TimeTicks& pts,
    const base::TimeTicks& dts) {
  if (!unit_header_data) {
    DCHECK_EQ(0u, unit_header_size);
    unit_header_data = unit_data;
  }

  uint8_t header_data[pes::kPacketHeaderMaxSize];
  size_t header_size = pes::FillInPacketHeader(header_data, stream_id, pts, dts,
                                               unit_header_size, unit_size);
  DCHECK_LE(header_size, sizeof(header_data));
  return WiFiDisplayElementaryStreamPacket(header_data, header_size,
                                           unit_header_data, unit_header_size,
                                           unit_data, unit_size);
}

}  // namespace extensions
