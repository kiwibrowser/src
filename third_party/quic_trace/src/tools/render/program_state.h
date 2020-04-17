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

#ifndef THIRD_PARTY_QUIC_TRACE_TOOLS_PROGRAM_STATE_H_
#define THIRD_PARTY_QUIC_TRACE_TOOLS_PROGRAM_STATE_H_

#include "tools/render/geometry_util.h"
#include "tools/render/sdl_util.h"

namespace quic_trace {
namespace render {

extern const char* kProgramStateData;

// The global state of the program that every component and shader has access
// to.  There are some requirements regarding to how the format of this struct
// should be altered:
//  1. It should be kept in sync with the GLSL definition in .cc file.
//  2. There are some alignment rules that you have to follow, which are
//     somewhat elaborate (search "std140" for more details), but the
//     relevant part is that it's 4 bytes for floats and 8 bytes for 2D vectors.
struct ProgramStateData {
  // Size of the program window in pixels.
  vec2 window = vec2{1280.f, 720.f};
  // Starting point, in axis units (us/bytes), of the part of the trace that is
  // currently shown.
  vec2 offset = vec2{0.f, 0.f};
  // Size of the portion of the trace that is currently shown, in pixels.
  vec2 viewport = vec2{0.f, 0.f};
  // The factor by which UI elements need to be scaled due to the DPI.
  alignas(sizeof(float)) float dpi_scale = 1.f;
};

// A wrapper around ProgramStateData for the consumers of the data.  Allows
// access to the members of ProgramStateData as well as the copy of them in GPU
// memory.
class ProgramState {
 public:
  ProgramState(const ProgramStateData* data);
  ProgramState(const ProgramState&) = delete;
  ProgramState& operator=(const ProgramState&) = delete;

  const vec2& window() const { return data_->window; }
  const vec2& offset() const { return data_->offset; }
  const vec2& viewport() const { return data_->viewport; }
  float dpi_scale() const { return data_->dpi_scale; }

  int ScaleForDpi(int value) const {
    return std::round(data_->dpi_scale * value);
  }

  // Binds the Uniform Buffer Object with |data_| onto the specified shader
  // program.
  void Bind(const GlProgram& program) const;
  // Copies |data_| into |buffer_|.
  void Refresh();

 private:
  // Data on host.
  const ProgramStateData* data_;
  // Handle to data on the device.
  GlUniformBuffer buffer_;
};

}  // namespace render
}  // namespace quic_trace

#endif  // THIRD_PARTY_QUIC_TRACE_TOOLS_PROGRAM_STATE_H_
