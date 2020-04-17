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

#ifndef THIRD_PARTY_QUIC_TRACE_TOOLS_LAYOUT_CONSTANTS_H_
#define THIRD_PARTY_QUIC_TRACE_TOOLS_LAYOUT_CONSTANTS_H_

#include "tools/render/geometry_util.h"

namespace quic_trace {
namespace render {

// Constants that describe part of the QUIC rendering program layout which are
// used in multiple parts of the program.
constexpr vec2 TraceMargin(float dpi_scale) {
  return dpi_scale * vec2(80.f, 50.f);
}

static constexpr float kSentPacketDurationMs = 1000;

}  // namespace render
}  // namespace quic_trace

#endif  // THIRD_PARTY_QUIC_TRACE_TOOLS_LAYOUT_CONSTANTS_H_
