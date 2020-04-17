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

#include "tools/render/rectangle_renderer.h"

#include <array>

#include "absl/algorithm/container.h"

namespace quic_trace {
namespace render {

namespace {

const char* kVertexShader = R"(
in vec2 coord;
in vec4 color_in;
out vec4 color_mid;

void main(void) {
  gl_Position = windowToGl(coord);
  color_mid = color_in;
}
)";

const char* kFragmentShader = R"(
in vec4 color_mid;
out vec4 color_out;

void main(void) {
  color_out = color_mid;
}
)";

const char* kLineFragmentShader = R"(
out vec4 color_out;
void main(void) {
  color_out = vec4(0, 0, 0, 1);
}
)";

}  // namespace

RectangleRenderer::RectangleRenderer(const ProgramState* state)
    : state_(state),
      shader_(kVertexShader, kFragmentShader),
      line_shader_(kVertexShader, kLineFragmentShader) {}

void RectangleRenderer::AddRectangle(Box box, uint32_t rgba) {
  rgba = __builtin_bswap32(rgba);
  // This looks like this:
  //   2  3
  //   0  1
  points_.push_back(Point{box.origin.x, box.origin.y, rgba});               // 0
  points_.push_back(Point{box.origin.x + box.size.x, box.origin.y, rgba});  // 1
  points_.push_back(Point{box.origin.x, box.origin.y + box.size.y, rgba});  // 2
  points_.push_back(
      Point{box.origin.x + box.size.x, box.origin.y + box.size.y, rgba});  // 3
}

void RectangleRenderer::AddRectangleWithBorder(Box box, uint32_t rgba) {
  const size_t point_buffer_offset = points_.size();
  AddRectangle(box, rgba);

  const std::array<uint16_t, 8> lines = {0, 1, 1, 3, 3, 2, 2, 0};
  line_indices_.resize(line_indices_.size() + 8);
  absl::c_transform(lines, line_indices_.end() - 8,
                    [=](uint16_t i) { return i + point_buffer_offset; });
}

void RectangleRenderer::Render() {
  GlVertexBuffer point_buffer;
  glBufferData(GL_ARRAY_BUFFER, points_.size() * sizeof(Point), points_.data(),
               GL_STREAM_DRAW);

  const size_t rectangle_count = points_.size() / 4;
  const size_t point_count = rectangle_count * 6;
  CHECK(point_count < std::numeric_limits<uint16_t>::max());

  // Transform points of each rectangle into a pair of triangles.
  auto indices = absl::make_unique<uint16_t[]>(point_count);
  for (int i = 0; i < rectangle_count; i++) {
    indices[6 * i + 0] = 4 * i + 0;
    indices[6 * i + 1] = 4 * i + 1;
    indices[6 * i + 2] = 4 * i + 2;
    indices[6 * i + 3] = 4 * i + 2;
    indices[6 * i + 4] = 4 * i + 3;
    indices[6 * i + 5] = 4 * i + 1;
  }

  glUseProgram(*shader_);
  state_->Bind(shader_);

  {
    GlVertexArray array;
    glBindVertexArray(*array);

    GlElementBuffer index_buffer;
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, point_count * sizeof(indices[0]),
                 indices.get(), GL_STREAM_DRAW);

    GlVertexArrayAttrib coord(shader_, "coord");
    glVertexAttribPointer(*coord, 2, GL_FLOAT, GL_FALSE, sizeof(Point), 0);
    GlVertexArrayAttrib color(shader_, "color_in");
    glVertexAttribPointer(*color, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Point),
                          (void*)offsetof(Point, rgba));

    glDrawElements(GL_TRIANGLES, point_count, GL_UNSIGNED_SHORT, nullptr);
    points_.clear();
  }

  glUseProgram(*line_shader_);
  state_->Bind(line_shader_);
  {
    GlVertexArray array;
    glBindVertexArray(*array);

    GlElementBuffer index_buffer;
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 line_indices_.size() * sizeof(line_indices_[0]),
                 line_indices_.data(), GL_STREAM_DRAW);

    GlVertexArrayAttrib coord(shader_, "coord");
    glVertexAttribPointer(*coord, 2, GL_FLOAT, GL_FALSE, sizeof(Point), 0);

    glDrawElements(GL_LINES, line_indices_.size(), GL_UNSIGNED_SHORT, nullptr);
    line_indices_.clear();
  }
}

}  // namespace render
}  // namespace quic_trace
