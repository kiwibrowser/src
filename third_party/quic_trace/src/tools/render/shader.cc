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

#include "tools/render/shader.h"

#include "absl/strings/str_cat.h"
#include "tools/render/program_state.h"

namespace quic_trace {
namespace render {

const char* kShaderPreamble = "#version 150 core";

const char* kShaderLibrary = R"(
// Transforms the window coordinates into the GL coordinates.
vec4 windowToGl(vec2 window_point) {
  return vec4((window_point / program_state.window) * 2 - vec2(1, 1), 0, 1);
}
)";

static std::string PreprocessShader(const char* shader) {
  return absl::StrCat(kShaderPreamble, "\n", kProgramStateData, "\n",
                      kShaderLibrary, "\n", shader);
}

Shader::Shader(const char* vertex_code, const char* fragment_code)
    : Shader(vertex_code, nullptr, fragment_code) {}

Shader::Shader(const char* vertex_code,
               const char* geometry_code,
               const char* fragment_code)
    : vertex_shader_(GL_VERTEX_SHADER), fragment_shader_(GL_FRAGMENT_SHADER) {
  vertex_shader_.CompileOrDie(PreprocessShader(vertex_code).c_str());
  Attach(vertex_shader_);

  fragment_shader_.CompileOrDie(PreprocessShader(fragment_code).c_str());
  Attach(fragment_shader_);

  if (geometry_code != nullptr) {
    geometry_shader_.emplace(GL_GEOMETRY_SHADER);
    geometry_shader_->CompileOrDie(PreprocessShader(geometry_code).c_str());
    Attach(*geometry_shader_);
  }

  CHECK(Link());
}

}  // namespace render
}  // namespace quic_trace
