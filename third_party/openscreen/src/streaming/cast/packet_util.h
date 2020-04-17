// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef STREAMING_CAST_PACKET_UTIL_H_
#define STREAMING_CAST_PACKET_UTIL_H_

#include <utility>

#include "absl/types/span.h"
#include "osp_base/big_endian.h"
#include "streaming/cast/ssrc.h"

namespace openscreen {
namespace cast_streaming {

// Reads a field from the start of the given span and advances the span to point
// just after the field.
template <typename Integer>
inline Integer ConsumeField(absl::Span<const uint8_t>* in) {
  const Integer result = ReadBigEndian<Integer>(in->data());
  in->remove_prefix(sizeof(Integer));
  return result;
}

// Writes a field at the start of the given span and advances the span to point
// just after the field.
template <typename Integer>
inline void AppendField(Integer value, absl::Span<uint8_t>* out) {
  WriteBigEndian<Integer>(value, out->data());
  out->remove_prefix(sizeof(Integer));
}

// Returns a bitmask for a field having the given number of bits. For example,
// FieldBitmask<uint8_t>(5) returns 0b00011111.
template <typename Integer>
constexpr Integer FieldBitmask(unsigned field_size_in_bits) {
  return (Integer{1} << field_size_in_bits) - 1;
}

// Performs a quick-scan of the packet data for the purposes of routing it to an
// appropriate parser. Identifies whether the packet is a RTP packet, RTCP
// packet, or unknown; and provides the originator's SSRC. This only performs a
// very quick scan of the packet data, and does not guarantee that a full parse
// will later succeed.
enum class ApparentPacketType { UNKNOWN, RTP, RTCP };
std::pair<ApparentPacketType, Ssrc> InspectPacketForRouting(
    absl::Span<const uint8_t> packet);

}  // namespace cast_streaming
}  // namespace openscreen

#endif  // STREAMING_CAST_PACKET_UTIL_H_
