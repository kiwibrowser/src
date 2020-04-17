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

#ifndef THIRD_PARTY_QUIC_TRACE_TOOLS_TRACE_RENDERER_H_
#define THIRD_PARTY_QUIC_TRACE_TOOLS_TRACE_RENDERER_H_

#include <cstdint>

#include "absl/types/optional.h"
#include "tools/render/program_state.h"
#include "tools/render/sdl_util.h"
#include "tools/render/shader.h"

namespace quic_trace {
namespace render {

enum class PacketType { SENT, ACKED, LOST, APP_LIMITED };

// Draws the trace on the current OpenGL context.
class TraceRenderer {
 public:
  TraceRenderer(const ProgramState* state);

  // Preallocate the packet buffer.
  void PacketCountHint(size_t count);
  // Add a new packet to be drawn.
  void AddPacket(uint64_t time,
                 uint64_t offset,
                 uint64_t size,
                 PacketType type);
  // Uploads all of the packets to the GPU and frees the local buffer.  Must be
  // called before Render().
  void FinishPackets();

  // Actually draw the trace.
  void Render();

  float max_x() const { return max_x_; }
  float max_y() const { return max_y_; }

  // Sets the packet to highlight on trace.
  void set_highlighted_packet(int highlighted_packet) {
    highlighted_packet_ = highlighted_packet;
  }

 private:
  // Packet metadata as uploaded onto the GPU.
  struct Packet {
    float time;
    float offset;
    float size;
    float kind;
  };

  std::vector<Packet> packet_buffer_;
  bool buffer_ready_ = false;
  GlVertexBuffer buffer_;
  size_t packet_count_;
  GlVertexArray array_;

  const ProgramState* state_;
  Shader shader_;

  float max_x_ = 0.f;
  float max_y_ = 0.f;

  int highlighted_packet_ = -1;
};

}  // namespace render
}  // namespace quic_trace

#endif  // THIRD_PARTY_QUIC_TRACE_TOOLS_TRACE_RENDERER_H_
