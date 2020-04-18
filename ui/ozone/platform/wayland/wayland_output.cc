// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/wayland/wayland_output.h"

#include <wayland-client.h>

#include "ui/gfx/color_space.h"
#include "ui/ozone/platform/wayland/wayland_connection.h"

namespace ui {

WaylandOutput::WaylandOutput(const int64_t display_id, wl_output* output)
    : display_id_(display_id), output_(output), observer_(nullptr) {
  static const wl_output_listener output_listener = {
      &WaylandOutput::OutputHandleGeometry, &WaylandOutput::OutputHandleMode,
  };
  wl_output_add_listener(output, &output_listener, this);
}

WaylandOutput::~WaylandOutput() {}

void WaylandOutput::SetObserver(Observer* observer) {
  observer_ = observer;
  if (current_mode_)
    observer_->OnOutputReadyForUse();
}

void WaylandOutput::GetDisplaysSnapshot(display::GetDisplaysCallback callback) {
  std::vector<display::DisplaySnapshot*> snapshot;
  snapshot.push_back(current_snapshot_.get());
  std::move(callback).Run(snapshot);
}

// static
void WaylandOutput::OutputHandleGeometry(void* data,
                                         wl_output* output,
                                         int32_t x,
                                         int32_t y,
                                         int32_t physical_width,
                                         int32_t physical_height,
                                         int32_t subpixel,
                                         const char* make,
                                         const char* model,
                                         int32_t output_transform) {
  WaylandOutput* wayland_output = static_cast<WaylandOutput*>(data);
  wayland_output->current_snapshot_.reset(new display::DisplaySnapshot(
      wayland_output->display_id_, gfx::Point(x, y),
      gfx::Size(physical_width, physical_height),
      display::DisplayConnectionType::DISPLAY_CONNECTION_TYPE_NONE, false,
      false, false, gfx::ColorSpace(), model, base::FilePath(),
      display::DisplaySnapshot::DisplayModeList(), std::vector<uint8_t>(),
      nullptr, nullptr, 0, 0, gfx::Size()));
}

// static
void WaylandOutput::OutputHandleMode(void* data,
                                     wl_output* wl_output,
                                     uint32_t flags,
                                     int32_t width,
                                     int32_t height,
                                     int32_t refresh) {
  WaylandOutput* output = static_cast<WaylandOutput*>(data);

  if (flags & WL_OUTPUT_MODE_CURRENT) {
    std::unique_ptr<display::DisplayMode> previous_mode =
        std::move(output->current_mode_);
    output->current_mode_.reset(
        new display::DisplayMode(gfx::Size(width, height), false, refresh));
    output->current_snapshot_->set_current_mode(output->current_mode_.get());

    if (output->observer())
      output->observer()->OnOutputReadyForUse();
  }
}

}  // namespace ui
