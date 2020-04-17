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

#ifndef THIRD_PARTY_QUIC_TRACE_TOOLS_SHADER_H_
#define THIRD_PARTY_QUIC_TRACE_TOOLS_SHADER_H_

#include "absl/types/optional.h"
#include "tools/render/sdl_util.h"

namespace quic_trace {
namespace render {

// A utility class that wraps a GlProgram and all GlShader objects in it.  This
// class also does some preprocessing on shader source: it adds the #version
// header and the shared definition of uniform buffer and windowToGl()
// subroutine.
class Shader : public GlProgram {
 public:
  Shader(const char* vertex_code, const char* fragment_code);
  Shader(const char* vertex_code,
         const char* geometry_code,
         const char* fragment_code);

 private:
  GlShader vertex_shader_;
  GlShader fragment_shader_;
  absl::optional<GlShader> geometry_shader_;
};

}  // namespace render
}  // namespace quic_trace

#endif  // THIRD_PARTY_QUIC_TRACE_TOOLS_SHADER_H_
