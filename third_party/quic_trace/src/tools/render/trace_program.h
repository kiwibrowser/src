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

#ifndef THIRD_PARTY_QUIC_TRACE_TOOLS_TRACE_PROGRAM_H_
#define THIRD_PARTY_QUIC_TRACE_TOOLS_TRACE_PROGRAM_H_

#include "absl/time/time.h"
#include "lib/quic_trace.pb.h"
#include "tools/render/axis_renderer.h"
#include "tools/render/processed_trace.h"
#include "tools/render/program_state.h"
#include "tools/render/rectangle_renderer.h"
#include "tools/render/sdl_util.h"
#include "tools/render/table.h"
#include "tools/render/text.h"
#include "tools/render/trace_renderer.h"

namespace quic_trace {
namespace render {

// The framerate with respect to which all of the animations are scaled.  Equal
// to 60 frames per second.
constexpr absl::Duration kReferenceFrameDuration = absl::Microseconds(16667);

// Top-level code for the trace rendering tool.  Handles loading the trace from
// proto, the rendering and event loop, and the input events themselves.
class TraceProgram {
 public:
  TraceProgram();

  // Loads the trace into the renderer buffer.
  void LoadTrace(std::unique_ptr<Trace> trace);

  // Handle events and redraw the trace accordingly.
  void Loop();

 private:
  ProgramStateData state_;

  ScopedSDL sdl_;
  ScopedSdlWindow window_;
  OpenGlContext context_;

  std::unique_ptr<ProcessedTrace> trace_;

  std::unique_ptr<ProgramState> state_buffer_;
  std::unique_ptr<TraceRenderer> renderer_;
  std::unique_ptr<TextRenderer> text_renderer_;
  std::unique_ptr<AxisRenderer> axis_renderer_;
  std::unique_ptr<RectangleRenderer> rectangle_renderer_;

  bool quit_ = false;

  // If true, panning is currently in progress.
  bool panning_ = false;
  vec2 panning_last_pos_;
  bool zooming_ = false;
  int zoom_start_x_;
  int zoom_start_y_;
  bool summary_ = false;
  int summary_start_x_;
  absl::optional<Table> summary_table_;
  bool show_online_help_ = false;

  // On OS X, the window has different size for rendering and for mouse input
  // purposes.  This vector contains the factor we use to translate mouse
  // coordinates into the rendering coordinates.
  vec2 input_scale_;

  // An EWMA-filtered frame duration, used for scaling animations.
  absl::Duration frame_duration_ = kReferenceFrameDuration;

  // Scales a value with respect to framerate so that if you add |x| to some
  // variable every frame, the effect is always same numerically as if the
  // program is running at 60 frames per second.
  float ScaleAdditiveFactor(float x);
  // Same as above, except |k| is being multiplied instead of added.
  float ScaleMultiplicativeFactor(float k);
  // Zooms in or out, depending on the sign.  For example, if |zoom| is +0.98,
  // the size of the viewport would be 98% of what it was before, and -0.98
  // would perform an inverse transform.
  void Zoom(float zoom);

  void UpdateWindowSize();

  // Handle input and window events.
  void PollEvents();
  // Handle keyboard inputs.
  void PollKeyboard();
  // Handle mouse inputs .
  void PollMouse();
  // Handle panning via mouse.
  void HandlePanning(bool pressed, int x, int y);
  void HandleZooming(bool pressed, int x, int y);
  void HandleSummary(bool pressed, int x);
  void HandleMouseover(int x, int y);
  // Ensure that the currently rendered part of the trace stays within the
  // bounds of the trace.
  void EnsureBounds();

  // Converts a relative coordinate window vector into an appropriate trace
  // coordinate vector.
  vec2 WindowToTraceRelative(vec2 vector);
  // Converts an absolute coordinate window vector into an appropriate trace
  // coordinate vector.
  vec2 WindowToTraceCoordinates(vec2 point);
  Box WindowToTraceCoordinates(Box box);
  Box TraceBounds();

  // Shows the current framerate of the program in the corner if --show_fps is
  // enabled.
  void MaybeShowFramerate();
  Table GenerateOnlineHelp();
  // Draws all of the tables on the right side of the window.
  void DrawRightSideTables();
};

}  // namespace render
}  // namespace quic_trace

#endif  // THIRD_PARTY_QUIC_TRACE_TOOLS_TRACE_PROGRAM_H_
