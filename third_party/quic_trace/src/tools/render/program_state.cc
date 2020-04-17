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

#include "tools/render/program_state.h"

#include "tools/render/sdl_util.h"

namespace quic_trace {
namespace render {

// This has to mirror the definition of ProgramStateData in the .h file.
const char* kProgramStateData = R"(
layout(std140) uniform ProgramStateBlock {
  vec2 window;
  vec2 offset;
  vec2 viewport;
  float dpi_scale;
} program_state;
)";

ProgramState::ProgramState(const ProgramStateData* data) : data_(data) {
  glBufferData(GL_UNIFORM_BUFFER, sizeof(*data), data, GL_DYNAMIC_COPY);
}

void ProgramState::Bind(const GlProgram& program) const {
  GLuint block_index = glGetUniformBlockIndex(*program, "ProgramStateBlock");
  CHECK_NE(block_index, GL_INVALID_INDEX);
  glUniformBlockBinding(*program, block_index, 1);
  glBindBufferBase(GL_UNIFORM_BUFFER, 1, *buffer_);
}

void ProgramState::Refresh() {
  glBindBuffer(GL_UNIFORM_BUFFER, *buffer_);
  glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(*data_), data_);
}

}  // namespace render
}  // namespace quic_trace
