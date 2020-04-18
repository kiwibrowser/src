// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/platform_display_mirror.h"

#include "components/viz/common/surfaces/surface_id.h"
#include "components/viz/common/surfaces/surface_info.h"
#include "components/viz/host/renderer_settings_creation.h"
#include "services/ui/ws/display.h"
#include "services/ui/ws/display_manager.h"
#include "services/ui/ws/frame_generator.h"
#include "services/ui/ws/window_server.h"
#include "ui/platform_window/platform_window.h"

namespace ui {
namespace ws {

PlatformDisplayMirror::PlatformDisplayMirror(
    const display::Display& display,
    const display::ViewportMetrics& metrics,
    WindowServer* window_server,
    Display* display_to_mirror)
    : display_(display),
      metrics_(metrics),
      window_server_(window_server),
      display_to_mirror_(display_to_mirror) {
  DCHECK(display_to_mirror_);
  frame_sink_id_ = window_server->display_manager()->GetAndAdvanceNextRootId();

  // Create a new platform window to display the mirror destination content.
  platform_window_ = CreatePlatformWindow(this, metrics_.bounds_in_pixels);
  platform_window_->Show();
}

PlatformDisplayMirror::~PlatformDisplayMirror() = default;

void PlatformDisplayMirror::Init(PlatformDisplayDelegate* delegate) {}

void PlatformDisplayMirror::SetViewportSize(const gfx::Size& size) {}

void PlatformDisplayMirror::SetTitle(const base::string16& title) {}

void PlatformDisplayMirror::SetCapture() {}

void PlatformDisplayMirror::ReleaseCapture() {}

void PlatformDisplayMirror::SetCursor(const ui::CursorData& cursor) {}

void PlatformDisplayMirror::MoveCursorTo(
    const gfx::Point& window_pixel_location) {}

void PlatformDisplayMirror::SetCursorSize(const ui::CursorSize& cursor_size) {}

void PlatformDisplayMirror::ConfineCursorToBounds(
    const gfx::Rect& pixel_bounds) {}

void PlatformDisplayMirror::UpdateTextInputState(
    const ui::TextInputState& state) {}

void PlatformDisplayMirror::SetImeVisibility(bool visible) {}

void PlatformDisplayMirror::UpdateViewportMetrics(
    const display::ViewportMetrics& metrics) {
  metrics_ = metrics;
}

const display::ViewportMetrics& PlatformDisplayMirror::GetViewportMetrics() {
  return metrics_;
}

gfx::AcceleratedWidget PlatformDisplayMirror::GetAcceleratedWidget() const {
  return widget_;
}

FrameGenerator* PlatformDisplayMirror::GetFrameGenerator() {
  return frame_generator_.get();
}

EventSink* PlatformDisplayMirror::GetEventSink() {
  return nullptr;
}

void PlatformDisplayMirror::SetCursorConfig(display::Display::Rotation rotation,
                                            float scale) {}

void PlatformDisplayMirror::OnBoundsChanged(const gfx::Rect& new_bounds) {}

void PlatformDisplayMirror::OnDamageRect(const gfx::Rect& damaged_region) {}

void PlatformDisplayMirror::DispatchEvent(ui::Event* event) {}

void PlatformDisplayMirror::OnCloseRequest() {}

void PlatformDisplayMirror::OnClosed() {}

void PlatformDisplayMirror::OnWindowStateChanged(
    ui::PlatformWindowState new_state) {}

void PlatformDisplayMirror::OnLostCapture() {}

void PlatformDisplayMirror::OnAcceleratedWidgetAvailable(
    gfx::AcceleratedWidget widget,
    float device_scale_factor) {
  DCHECK_EQ(gfx::kNullAcceleratedWidget, widget_);
  widget_ = widget;

  // Create a CompositorFrameSink for this display, using the widget's surface.
  viz::mojom::CompositorFrameSinkAssociatedPtr compositor_frame_sink;
  viz::mojom::CompositorFrameSinkClientPtr compositor_frame_sink_client;
  viz::mojom::CompositorFrameSinkClientRequest
      compositor_frame_sink_client_request =
          mojo::MakeRequest(&compositor_frame_sink_client);
  window_server_->GetVizHostProxy()->RegisterFrameSinkId(frame_sink_id_, this);

  // TODO(ccameron): Bind |display_client| to support macOS? (maybe not needed)
  viz::mojom::DisplayPrivateAssociatedPtr display_private;
  viz::mojom::DisplayClientPtr display_client;
  viz::mojom::DisplayClientRequest display_client_request =
      mojo::MakeRequest(&display_client);

  auto params = viz::mojom::RootCompositorFrameSinkParams::New();
  params->frame_sink_id = frame_sink_id_;
  params->widget = widget_;
  params->renderer_settings = viz::CreateRendererSettings();
  params->compositor_frame_sink = mojo::MakeRequest(&compositor_frame_sink);
  params->compositor_frame_sink_client =
      compositor_frame_sink_client.PassInterface();
  params->display_private = mojo::MakeRequest(&display_private);
  params->display_client = display_client.PassInterface();
  window_server_->GetVizHostProxy()->CreateRootCompositorFrameSink(
      std::move(params));

  // Make a FrameGenerator that references |display_to_mirror_|'s surface id.
  display_private->SetDisplayVisible(true);
  frame_generator_ = std::make_unique<FrameGenerator>();
  auto frame_sink_client_binding =
      std::make_unique<CompositorFrameSinkClientBinding>(
          frame_generator_.get(),
          std::move(compositor_frame_sink_client_request),
          std::move(compositor_frame_sink), std::move(display_private));
  frame_generator_->Bind(std::move(frame_sink_client_binding));

  frame_generator_->OnWindowSizeChanged(metrics_.bounds_in_pixels.size());
  frame_generator_->SetDeviceScaleFactor(metrics_.device_scale_factor);
  frame_generator_->set_scale_and_center(true);

  // Pass the surface info for the mirror source display to the frame generator,
  // the id is not available if the source display init is not yet complete.
  const viz::SurfaceInfo& info = display_to_mirror_->platform_display()
                                     ->GetFrameGenerator()
                                     ->window_manager_surface_info();
  if (info.id().is_valid())
    frame_generator_->SetEmbeddedSurface(info);
}

void PlatformDisplayMirror::OnAcceleratedWidgetDestroying() {}

void PlatformDisplayMirror::OnAcceleratedWidgetDestroyed() {}

void PlatformDisplayMirror::OnActivationChanged(bool active) {}

void PlatformDisplayMirror::OnFirstSurfaceActivation(
    const viz::SurfaceInfo& surface_info) {}

void PlatformDisplayMirror::OnFrameTokenChanged(uint32_t frame_token) {}

}  // namespace ws
}  // namespace ui
