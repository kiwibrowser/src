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

#include "tools/render/trace_program.h"

#include "gflags/gflags.h"
#include "absl/time/clock.h"
#include "tools/render/layout_constants.h"

DEFINE_bool(show_fps, false, "Show the current framerate of the program");
DEFINE_bool(vsync, true, "Enables vsync");

DEFINE_double(mouseover_threshold,
              3.0,
              "The minimum size of a single packet (in fractional pixels) that "
              "causes the packet information box being showed");

namespace quic_trace {
namespace render {

TraceProgram::TraceProgram()
    : window_(SDL_CreateWindow(
          "QUIC trace viewer",
          0,
          0,
          state_.window.x,
          state_.window.y,
          SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI)),
      context_(*window_) {
  UpdateWindowSize();
  state_buffer_ = absl::make_unique<ProgramState>(&state_);
  renderer_ = absl::make_unique<TraceRenderer>(state_buffer_.get());
  text_renderer_ = absl::make_unique<TextRenderer>(state_buffer_.get());
  axis_renderer_ = absl::make_unique<AxisRenderer>(text_renderer_.get(),
                                                   state_buffer_.get());
  rectangle_renderer_ =
      absl::make_unique<RectangleRenderer>(state_buffer_.get());

  SDL_GL_SetSwapInterval(FLAGS_vsync ? 1 : 0);
  SDL_SetWindowMinimumSize(*window_, 640, 480);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void TraceProgram::LoadTrace(std::unique_ptr<Trace> trace) {
  std::stable_sort(
      trace->mutable_events()->begin(), trace->mutable_events()->end(),
      [](const Event& a, const Event& b) { return a.time_us() < b.time_us(); });
  trace_ = absl::make_unique<ProcessedTrace>(std::move(trace), renderer_.get());
  state_.viewport.x = renderer_->max_x();
  state_.viewport.y = renderer_->max_y();
}

void TraceProgram::Loop() {
  while (!quit_) {
    absl::Time frame_start = absl::Now();
    PollEvents();
    PollKeyboard();
    PollMouse();
    EnsureBounds();

    state_buffer_->Refresh();

    // Render.
    glClearColor(1.f, 1.f, 1.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    // Note that the order of calls below determines what is drawn on top of
    // what.
    renderer_->Render();
    axis_renderer_->Render();
    MaybeShowFramerate();
    DrawRightSideTables();
    // The batch object renderers should be called last.
    rectangle_renderer_->Render();
    text_renderer_->DrawAll();
    SDL_GL_SwapWindow(*window_);

    absl::Time frame_end = absl::Now();
    frame_duration_ = 0.25 * (frame_end - frame_start) + 0.75 * frame_duration_;
  }
}

float TraceProgram::ScaleAdditiveFactor(float x) {
  return x * absl::ToDoubleSeconds(frame_duration_) /
         absl::ToDoubleSeconds(kReferenceFrameDuration);
}

float TraceProgram::ScaleMultiplicativeFactor(float k) {
  return std::pow(k, absl::ToDoubleSeconds(frame_duration_) /
                         absl::ToDoubleSeconds(kReferenceFrameDuration));
}

void TraceProgram::Zoom(float zoom) {
  float zoom_factor = std::abs(zoom);
  float sign = std::copysign(1.f, zoom);

  // Ensure that the central point doesn't move.
  state_.offset.x += sign * (1 - zoom_factor) * state_.viewport.x / 2;
  state_.offset.y += sign * (1 - zoom_factor) * state_.viewport.y / 2;

  state_.viewport.x *= std::pow(zoom_factor, sign);
  state_.viewport.y *= std::pow(zoom_factor, sign);
}

void TraceProgram::UpdateWindowSize() {
  int width, height;
  SDL_GL_GetDrawableSize(*window_, &width, &height);
  state_.window = vec2(width, height);
  glViewport(0, 0, width, height);

  int input_width, input_height;
  SDL_GetWindowSize(*window_, &input_width, &input_height);
  input_scale_ = vec2((float)width / input_width, (float)height / input_height);

  const float kReferenceDpi = 100.f;
  float dpi;
  int result = SDL_GetDisplayDPI(SDL_GetWindowDisplayIndex(*window_), &dpi,
                                 nullptr, nullptr);
  if (result < 0) {
    LOG(WARNING) << "Failed to retrieve window DPI";
  }
  state_.dpi_scale = input_scale_.x * dpi / kReferenceDpi;
}

void TraceProgram::PollEvents() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
      case SDL_QUIT:
        quit_ = true;
        break;

      case SDL_WINDOWEVENT:
        if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
          UpdateWindowSize();
        }
        break;

      case SDL_KEYDOWN:
        if (event.key.keysym.scancode == SDL_SCANCODE_H) {
          show_online_help_ = !show_online_help_;
        }
        break;

      case SDL_MOUSEWHEEL: {
        int wheel_offset = event.wheel.y;
        if (wheel_offset == 0) {
          break;
        }
        if (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED) {
          wheel_offset *= -1;
        }
        // Note that this does not need to be scaled with framerate since
        // mousewheel events are discrete.
        Zoom(std::copysign(0.95f, wheel_offset));
        break;
      }
    }
  }
}

void TraceProgram::PollKeyboard() {
  const uint8_t* state = SDL_GetKeyboardState(nullptr);
  if (state[SDL_SCANCODE_Q]) {
    quit_ = true;
  }

  // Zoom handling.
  const float zoom_factor = ScaleMultiplicativeFactor(0.98);
  if (state[SDL_SCANCODE_Z]) {
    Zoom(+zoom_factor);
  }
  if (state[SDL_SCANCODE_X]) {
    Zoom(-zoom_factor);
  }
  if (state[SDL_SCANCODE_UP]) {
    state_.offset.y += ScaleAdditiveFactor(state_.viewport.y * 0.03);
  }
  if (state[SDL_SCANCODE_DOWN]) {
    state_.offset.y -= ScaleAdditiveFactor(state_.viewport.y * 0.03);
  }
  if (state[SDL_SCANCODE_LEFT]) {
    state_.offset.x -= ScaleAdditiveFactor(state_.viewport.x * 0.03);
  }
  if (state[SDL_SCANCODE_RIGHT]) {
    state_.offset.x += ScaleAdditiveFactor(state_.viewport.x * 0.03);
  }
  if (state[SDL_SCANCODE_R]) {
    absl::optional<Box> new_viewport =
        trace_->BoundContainedPackets(Box{state_.offset, state_.viewport});
    if (new_viewport) {
      state_.offset = new_viewport->origin;
      state_.viewport = new_viewport->size;
    }
  }
}

void TraceProgram::PollMouse() {
  int x, y;
  uint32_t buttons = SDL_GetMouseState(&x, &y);
  const uint8_t* state = SDL_GetKeyboardState(nullptr);
  bool shift = state[SDL_SCANCODE_LSHIFT] || state[SDL_SCANCODE_RSHIFT];
  x *= input_scale_.x;
  y *= input_scale_.y;

  HandlePanning((buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) && !shift, x,
                state_.window.y - y);
  HandleSummary((buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) && shift, x);
  HandleZooming(buttons & SDL_BUTTON(SDL_BUTTON_RIGHT), x, state_.window.y - y);
  HandleMouseover(x, state_.window.y - y);
}

void TraceProgram::HandlePanning(bool pressed, int x, int y) {
  if (!pressed) {
    panning_ = false;
    return;
  }

  if (!panning_) {
    panning_ = true;
    panning_last_pos_ = vec2(x, y);
    return;
  }

  state_.offset += WindowToTraceRelative(panning_last_pos_ - vec2(x, y));

  panning_last_pos_ = vec2(x, y);
}

void TraceProgram::HandleZooming(bool pressed, int x, int y) {
  if (!pressed && !zooming_) {
    return;
  }

  if (pressed && !zooming_) {
    zooming_ = true;
    zoom_start_x_ = x;
    zoom_start_y_ = y;
    return;
  }

  Box window_box = BoundingBox(vec2(x, y), vec2(zoom_start_x_, zoom_start_y_));
  // Ensure that the selection does not go out of the bounds of the trace view.
  window_box = IntersectBoxes(window_box, TraceBounds());

  if (pressed && zooming_) {
    // User is still selecting the area to zoom into.  Draw a transparent grey
    // rectangle to indicate the currently picked area.
    rectangle_renderer_->AddRectangle(window_box, 0x00000033);
    return;
  }

  // Actually zoom in.
  zooming_ = false;
  // Discard all attempts to zoom in into something smaller than 16x16 pixels,
  // as those are more liekly to be accidental.
  if (window_box.size.x < 16 || window_box.size.y < 16) {
    return;
  }

  const Box trace_box = WindowToTraceCoordinates(window_box);
  state_.viewport = trace_box.size;
  state_.offset = trace_box.origin;
}

void TraceProgram::HandleSummary(bool pressed, int x) {
  if (!pressed) {
    summary_ = false;
    return;
  }

  if (!summary_) {
    summary_ = true;
    summary_start_x_ = x;
    return;
  }

  Box window_box = BoundingBox(vec2(x, 0), vec2(summary_start_x_, 99999.f));
  // Ensure that the selection does not go out of the bounds of the trace view.
  window_box = IntersectBoxes(window_box, TraceBounds());
  if (window_box.size.x < 1) {
    return;
  }
  rectangle_renderer_->AddRectangle(window_box, 0x00000033);

  const Box selected = WindowToTraceCoordinates(window_box);

  summary_table_.emplace(state_buffer_.get(), text_renderer_.get(),
                         rectangle_renderer_.get());
  if (!trace_->SummaryTable(&*summary_table_, selected.origin.x,
                            selected.origin.x + selected.size.x)) {
    summary_table_ = absl::nullopt;
  }
}

void TraceProgram::HandleMouseover(int x, int y) {
  vec2 window_pos(x, y);
  if (!IsInside(window_pos, TraceBounds())) {
    return;
  }

  vec2 trace_pos = WindowToTraceCoordinates(window_pos);

  float packet_size_in_pixels =
      kSentPacketDurationMs / state_.viewport.x * state_.window.x;
  if (packet_size_in_pixels < FLAGS_mouseover_threshold) {
    renderer_->set_highlighted_packet(-1);
    return;
  }

  constexpr int kPixelMargin = 64;
  const vec2 margin = WindowToTraceRelative(vec2(kPixelMargin, kPixelMargin));
  ProcessedTrace::PacketSearchResult hovered_packet =
      trace_->FindPacketContainingPoint(trace_pos, margin);
  renderer_->set_highlighted_packet(hovered_packet.index);
  if (hovered_packet.event == nullptr) {
    return;
  }

  constexpr vec2 kMouseoverOffset = vec2(32, 32);
  Table table(state_buffer_.get(), text_renderer_.get(),
              rectangle_renderer_.get());
  trace_->FillTableForPacket(&table, hovered_packet.as_rendered,
                             hovered_packet.event);
  vec2 table_size = table.Layout();
  table.Draw(vec2(x, y) + kMouseoverOffset + 0 * table_size);
}

Table TraceProgram::GenerateOnlineHelp() {
  Table table(state_buffer_.get(), text_renderer_.get(),
              rectangle_renderer_.get());
  table.AddHeader("Help");
  table.AddRow("h", "Toggle help");
  table.AddRow("z", "Zoom in");
  table.AddRow("x", "Zoom out");
  table.AddRow("r", "Rescale");
  table.AddRow("Arrows", "Move");
  table.AddRow("LMouse", "Move");
  table.AddRow("RMouse", "Zoom");
  table.AddRow("Shift+LM", "Summary");
  return table;
}

void TraceProgram::EnsureBounds() {
  constexpr float kTimeMargin = 3000.f;
  constexpr float kOffsetMargin = 10 * 1350.f;
  state_.viewport.x =
      std::min(state_.viewport.x, renderer_->max_x() + 2 * kTimeMargin);
  state_.viewport.y =
      std::min(state_.viewport.y, renderer_->max_y() + 2 * kOffsetMargin);

  const float min_x = -kTimeMargin;
  const float min_y = -kOffsetMargin;
  const float max_x = renderer_->max_x() + kTimeMargin;
  const float max_y = renderer_->max_y() + kOffsetMargin;

  state_.offset.x = std::max(min_x, state_.offset.x);
  state_.offset.x = std::min(max_x - state_.viewport.x, state_.offset.x);
  state_.offset.y = std::max(min_y, state_.offset.y);
  state_.offset.y = std::min(max_y - state_.viewport.y, state_.offset.y);
}

vec2 TraceProgram::WindowToTraceRelative(vec2 vector) {
  const vec2 pixel_viewport = state_.window - 2 * TraceMargin(state_.dpi_scale);
  return vector * state_.viewport / pixel_viewport;
}

vec2 TraceProgram::WindowToTraceCoordinates(vec2 point) {
  return state_.offset +
         WindowToTraceRelative(point - TraceMargin(state_.dpi_scale));
}

Box TraceProgram::WindowToTraceCoordinates(Box box) {
  return BoundingBox(WindowToTraceCoordinates(box.origin),
                     WindowToTraceCoordinates(box.origin + box.size));
}

Box TraceProgram::TraceBounds() {
  return BoundingBox(TraceMargin(state_.dpi_scale),
                     state_.window - TraceMargin(state_.dpi_scale));
}

void TraceProgram::MaybeShowFramerate() {
  if (!FLAGS_show_fps) {
    return;
  }

  char buffer[16];
  snprintf(buffer, sizeof(buffer), "%.2ffps",
           1. / absl::ToDoubleSeconds(frame_duration_));
  std::shared_ptr<const Text> framerate = text_renderer_->RenderText(buffer);
  text_renderer_->AddText(framerate, 0, state_.window.y - framerate->height());
}

void TraceProgram::DrawRightSideTables() {
  float distance = 20.f * state_.dpi_scale;
  vec2 offset = state_.window - vec2(distance, distance);

  if (summary_table_.has_value()) {
    vec2 table_size = summary_table_->Layout();
    summary_table_->Draw(offset - table_size);
    offset.y -= table_size.y + distance;
  }

  if (show_online_help_) {
    Table online_help = GenerateOnlineHelp();
    vec2 table_size = online_help.Layout();
    online_help.Draw(offset - table_size);
    offset.y -= table_size.y + distance;
  }

  summary_table_ = absl::nullopt;
}

}  // namespace render
}  // namespace quic_trace
