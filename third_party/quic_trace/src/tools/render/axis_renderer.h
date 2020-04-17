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

#ifndef THIRD_PARTY_QUIC_TRACE_TOOLS_AXIS_RENDERER_H_
#define THIRD_PARTY_QUIC_TRACE_TOOLS_AXIS_RENDERER_H_

#include "tools/render/program_state.h"
#include "tools/render/sdl_util.h"
#include "tools/render/text.h"

namespace quic_trace {
namespace render {

// Renders the X and Y axis, together with ticks and labels along it.
class AxisRenderer {
 public:
  AxisRenderer(TextRenderer* text_factory, const ProgramState* state);

  void Render();

 private:
  Shader shader_;

  const ProgramState* state_;
  TextRenderer* text_renderer_;

  size_t reference_label_width_;
  size_t reference_label_height_;
};

}  // namespace render
}  // namespace quic_trace

#endif  // THIRD_PARTY_QUIC_TRACE_TOOLS_AXIS_RENDERER_H_
