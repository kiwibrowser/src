// Copyright 2018 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// A library that assigns a global numbering to the events in the trace.

#ifndef THIRD_PARTY_QUIC_TRACE_LIB_ANALYSIS_TRACE_NUMBERING_H_
#define THIRD_PARTY_QUIC_TRACE_LIB_ANALYSIS_TRACE_NUMBERING_H_

#include <cstdint>
#include <unordered_map>

#include "absl/container/flat_hash_map.h"
#include "lib/quic_trace.pb.h"

namespace quic_trace {

// Trace offset of a packet in the connection trace.
using TraceOffset = uint64_t;

struct Interval {
  TraceOffset offset;
  size_t size;
};

// Assigns a new connection offset to every sent packet, regardless of whether
// it's a retransmission or not.
//
// TODO(vasilvv): port QuicTcpLikeTraceConverter from QUIC for retransmission
// support.
class NumberingWithoutRetransmissions {
 public:
  Interval AssignTraceNumbering(const Event& event) {
    size_t size = 0;
    for (const Frame& frame : event.frames()) {
      if (frame.has_stream_frame_info()) {
        size += frame.stream_frame_info().length();
      }
    }
    TraceOffset offset = current_offset_;
    current_offset_ += size;
    offsets_.emplace(event.packet_number(), Interval{offset, size});
    return {offset, size};
  }

  Interval GetTraceNumbering(uint64_t packet_number) {
    auto it = offsets_.find(packet_number);
    return it != offsets_.end() ? it->second : Interval{0, 0};
  }

 private:
  TraceOffset current_offset_ = 0;
  absl::flat_hash_map<uint64_t, Interval> offsets_;
};

}  // namespace quic_trace

#endif  // THIRD_PARTY_QUIC_TRACE_LIB_ANALYSIS_TRACE_NUMBERING_H_
