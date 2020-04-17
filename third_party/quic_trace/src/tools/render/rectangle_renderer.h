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

#ifndef THIRD_PARTY_QUIC_TRACE_TOOLS_RECTANGLE_RENDERER_H_
#define THIRD_PARTY_QUIC_TRACE_TOOLS_RECTANGLE_RENDERER_H_

#include "tools/render/program_state.h"
#include "tools/render/shader.h"

namespace quic_trace {
namespace render {

// Draws bunch of rectangles of specified color.
class RectangleRenderer {
 public:
  RectangleRenderer(const ProgramState* state);

  void AddRectangle(Box box, uint32_t rgba);
  void AddRectangleWithBorder(Box box, uint32_t rgba);
  void Render();

 private:
  struct Point {
    float x;
    float y;
    uint32_t rgba;
  };
  std::vector<Point> points_;
  std::vector<uint16_t> line_indices_;

  const ProgramState* state_;
  Shader shader_;
  Shader line_shader_;
};

}  // namespace render
}  // namespace quic_trace

#endif  // THIRD_PARTY_QUIC_TRACE_TOOLS_RECTANGLE_RENDERER_H_
