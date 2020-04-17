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

#ifndef THIRD_PARTY_QUIC_TRACE_TOOLS_PROCESSED_TRACE_H_
#define THIRD_PARTY_QUIC_TRACE_TOOLS_PROCESSED_TRACE_H_

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "lib/analysis/trace_numbering.h"
#include "lib/quic_trace.pb.h"
#include "tools/render/table.h"
#include "tools/render/trace_renderer.h"

namespace quic_trace {
namespace render {

// Stores a QUIC trace with additional information required to show useful
// information about portions of the trace and individual packets.
class ProcessedTrace {
 public:
  struct PacketSearchResult {
    // Index of the packet that can be supplied to the renderer.
    ptrdiff_t index = -1;
    const Box* as_rendered = nullptr;
    const Event* event = nullptr;
  };

  // Preprocesses all of information in the trace and populates the |renderer|
  // object.
  ProcessedTrace(std::unique_ptr<Trace> trace, TraceRenderer* renderer);

  // Populates the summary table for (start_time, end_time) range, or returns
  // false if the range is empty.
  bool SummaryTable(Table* table, float start_time, float end_time);

  // Finds the packet containing the specified point and returns the offset in
  // the array of all packet boxes drawn, or -1 if it's outside any of the
  // points.  |margin| specifies the size (in trace units) by which every packet
  // box is extended for purpose of this search (if multiple boxes are matched,
  // the closest one is returned).
  PacketSearchResult FindPacketContainingPoint(vec2 point, vec2 margin);

  void FillTableForPacket(Table* table,
                          const Box* as_rendered,
                          const Event* packet);

  // For specified section of the graph, find a bounding box that contains all
  // of the packets in it, or return nullopt if none are contained.
  absl::optional<Box> BoundContainedPackets(Box boundary);

 private:
  struct RenderedPacket {
    Box box;
    const Event* packet;

    bool operator<(const RenderedPacket& other) const {
      return box.origin.x < other.box.origin.x;
    }
  };

  struct VectorHash {
    size_t operator()(vec2 vector) const {
      return std::hash<float>()(vector.x) + std::hash<float>()(vector.y);
    }
  };

  void AddPacket(TraceRenderer* renderer,
                 const Event& packet,
                 Interval interval,
                 PacketType type);

  std::unique_ptr<Trace> trace_;
  absl::flat_hash_set<uint64_t> packets_acked_;
  absl::flat_hash_set<uint64_t> packets_lost_;
  // Map from packet-as-drawn offset to the ack.  This is required because
  // unlike sent or lost packets, there could be many acks derived from the same
  // Event object.
  absl::flat_hash_map<vec2, uint64_t, VectorHash> acks_;
  std::vector<RenderedPacket> rendered_packets_;
};

}  // namespace render
}  // namespace quic_trace

#endif  // THIRD_PARTY_QUIC_TRACE_TOOLS_PROCESSED_TRACE_H_
