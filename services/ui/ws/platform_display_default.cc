// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/platform_display_default.h"

#include <utility>

#include "build/build_config.h"
#include "gpu/ipc/client/gpu_channel_host.h"
#include "services/ui/display/screen_manager.h"
#include "services/ui/public/interfaces/cursor/cursor_struct_traits.h"
#include "services/ui/ws/server_window.h"
#include "services/ui/ws/threaded_image_cursors.h"
#include "services/viz/privileged/interfaces/compositing/display_private.mojom.h"
#include "ui/display/display.h"
#include "ui/events/event.h"
#include "ui/events/event_utils.h"
#include "ui/platform_window/platform_ime_controller.h"
#include "ui/platform_window/platform_window.h"
#include "ui/platform_window/stub/stub_window.h"

#if defined(USE_OZONE)
#include "ui/events/ozone/chromeos/cursor_controller.h"
#endif

namespace ui {
namespace ws {

PlatformDisplayDefault::PlatformDisplayDefault(
    ServerWindow* root_window,
    const display::ViewportMetrics& metrics,
    std::unique_ptr<ThreadedImageCursors> image_cursors)
    : root_window_(root_window),
      image_cursors_(std::move(image_cursors)),
      metrics_(metrics),
      widget_(gfx::kNullAcceleratedWidget) {}

PlatformDisplayDefault::~PlatformDisplayDefault() {
#if defined(OS_CHROMEOS)
  ui::CursorController::GetInstance()->ClearCursorConfigForWindow(
      GetAcceleratedWidget());
#endif

  // Don't notify the delegate from the destructor.
  delegate_ = nullptr;

  frame_generator_.reset();
  image_cursors_.reset();
  // Destroy the PlatformWindow early on as it may call us back during
  // destruction and we want to be in a known state. But destroy the surface
  // and ThreadedImageCursors first because they can still be using the platform
  // window.
  platform_window_.reset();
}

EventSink* PlatformDisplayDefault::GetEventSink() {
  return delegate_->GetEventSink();
}

void PlatformDisplayDefault::Init(PlatformDisplayDelegate* delegate) {
  DCHECK(delegate);
  delegate_ = delegate;

  const gfx::Rect& bounds = metrics_.bounds_in_pixels;
  DCHECK(!bounds.size().IsEmpty());

  if (delegate_->GetDisplay().id() == display::kUnifiedDisplayId) {
    // Virtual unified displays use a StubWindow; see AshWindowTreeHostUnified.
    platform_window_ = std::make_unique<ui::StubWindow>(this, true, bounds);
  } else if (delegate_->GetDisplay().id() == display::kInvalidDisplayId) {
    // Unit tests may use kInvalidDisplayId to request a StubWindow for testing.
    platform_window_ = std::make_unique<ui::StubWindow>(this, false);
  } else {
    platform_window_ = CreatePlatformWindow(this, metrics_.bounds_in_pixels);
  }

  platform_window_->Show();

  if (image_cursors_) {
    image_cursors_->SetDisplay(delegate_->GetDisplay(),
                               metrics_.device_scale_factor);
  }
}

void PlatformDisplayDefault::SetViewportSize(const gfx::Size& size) {
  platform_window_->SetBounds(gfx::Rect(size));
}

void PlatformDisplayDefault::SetTitle(const base::string16& title) {
  platform_window_->SetTitle(title);
}

void PlatformDisplayDefault::SetCapture() {
  platform_window_->SetCapture();
}

void PlatformDisplayDefault::ReleaseCapture() {
  platform_window_->ReleaseCapture();
}

void PlatformDisplayDefault::SetCursor(const ui::CursorData& cursor_data) {
  if (!image_cursors_)
    return;

  image_cursors_->SetCursor(cursor_data, platform_window_.get());
}

void PlatformDisplayDefault::MoveCursorTo(
    const gfx::Point& window_pixel_location) {
  platform_window_->MoveCursorTo(window_pixel_location);
}

void PlatformDisplayDefault::SetCursorSize(const ui::CursorSize& cursor_size) {
  image_cursors_->SetCursorSize(cursor_size);
}

void PlatformDisplayDefault::ConfineCursorToBounds(
    const gfx::Rect& pixel_bounds) {
  platform_window_->ConfineCursorToBounds(pixel_bounds);
}

void PlatformDisplayDefault::UpdateTextInputState(
    const ui::TextInputState& state) {
  ui::PlatformImeController* ime = platform_window_->GetPlatformImeController();
  if (ime)
    ime->UpdateTextInputState(state);
}

void PlatformDisplayDefault::SetImeVisibility(bool visible) {
  ui::PlatformImeController* ime = platform_window_->GetPlatformImeController();
  if (ime)
    ime->SetImeVisibility(visible);
}

FrameGenerator* PlatformDisplayDefault::GetFrameGenerator() {
  return frame_generator_.get();
}

void PlatformDisplayDefault::UpdateViewportMetrics(
    const display::ViewportMetrics& metrics) {
  if (metrics_ == metrics)
    return;

  gfx::Rect bounds = platform_window_->GetBounds();
  if (bounds.size() != metrics.bounds_in_pixels.size()) {
    bounds.set_size(metrics.bounds_in_pixels.size());
    platform_window_->SetBounds(bounds);
  }

  metrics_ = metrics;
  if (frame_generator_) {
    frame_generator_->SetDeviceScaleFactor(metrics_.device_scale_factor);
    frame_generator_->OnWindowSizeChanged(metrics_.bounds_in_pixels.size());
  }
}

const display::ViewportMetrics& PlatformDisplayDefault::GetViewportMetrics() {
  return metrics_;
}

gfx::AcceleratedWidget PlatformDisplayDefault::GetAcceleratedWidget() const {
  return widget_;
}

void PlatformDisplayDefault::SetCursorConfig(
    display::Display::Rotation rotation,
    float scale) {
#if defined(OS_CHROMEOS)
  ui::CursorController::GetInstance()->SetCursorConfigForWindow(
      GetAcceleratedWidget(), rotation, scale);
#endif
}

void PlatformDisplayDefault::OnBoundsChanged(const gfx::Rect& new_bounds) {
  // We only care if the window size has changed.
  if (new_bounds.size() == metrics_.bounds_in_pixels.size())
    return;

  // TODO(tonikitoo): Handle the bounds changing in external window mode. The
  // window should be resized by the WS and it shouldn't involve ScreenManager.
}

void PlatformDisplayDefault::OnDamageRect(const gfx::Rect& damaged_region) {
  if (frame_generator_)
    frame_generator_->OnWindowDamaged();
}

void PlatformDisplayDefault::DispatchEvent(ui::Event* event) {
  // Mojo requires conversion of mouse and touch events to pointer events.
  if (event->IsMouseEvent()) {
    ui::PointerEvent pointer_event(*event->AsMouseEvent());
    SendEventToSink(&pointer_event);
  } else if (event->IsTouchEvent()) {
    ui::PointerEvent pointer_event(*event->AsTouchEvent());
    SendEventToSink(&pointer_event);
  } else {
    SendEventToSink(event);
  }
}

void PlatformDisplayDefault::OnCloseRequest() {
  // TODO(tonikitoo): Handle a close request in external window mode. The window
  // should be closed by the WS and it shouldn't involve ScreenManager.
  const int64_t display_id = delegate_->GetDisplay().id();
  display::ScreenManager::GetInstance()->RequestCloseDisplay(display_id);
}

void PlatformDisplayDefault::OnClosed() {}

void PlatformDisplayDefault::OnWindowStateChanged(
    ui::PlatformWindowState new_state) {}

void PlatformDisplayDefault::OnLostCapture() {
  delegate_->OnNativeCaptureLost();
}

void PlatformDisplayDefault::OnAcceleratedWidgetAvailable(
    gfx::AcceleratedWidget widget,
    float device_scale_factor) {
  // This will get called after Init() is called, either synchronously as part
  // of the Init() callstack or async after Init() has returned, depending on
  // the platform.
  DCHECK_EQ(gfx::kNullAcceleratedWidget, widget_);
  widget_ = widget;
  delegate_->OnAcceleratedWidgetAvailable();

  if (!delegate_->IsHostingViz())
    return;

  viz::mojom::CompositorFrameSinkAssociatedPtr compositor_frame_sink;
  viz::mojom::CompositorFrameSinkClientPtr compositor_frame_sink_client;
  viz::mojom::CompositorFrameSinkClientRequest
      compositor_frame_sink_client_request =
          mojo::MakeRequest(&compositor_frame_sink_client);

  // TODO(ccameron): |display_client| is not bound. This will need to
  // change to support macOS.
  viz::mojom::DisplayPrivateAssociatedPtr display_private;
  viz::mojom::DisplayClientPtr display_client;
  viz::mojom::DisplayClientRequest display_client_request =
      mojo::MakeRequest(&display_client);

  root_window_->CreateRootCompositorFrameSink(
      widget_, mojo::MakeRequest(&compositor_frame_sink),
      std::move(compositor_frame_sink_client),
      mojo::MakeRequest(&display_private), std::move(display_client));

  display_private->SetDisplayVisible(true);
  frame_generator_ = std::make_unique<FrameGenerator>();
  auto frame_sink_client_binding =
      std::make_unique<CompositorFrameSinkClientBinding>(
          frame_generator_.get(),
          std::move(compositor_frame_sink_client_request),
          std::move(compositor_frame_sink), std::move(display_private));
  frame_generator_->Bind(std::move(frame_sink_client_binding));
  frame_generator_->OnWindowSizeChanged(root_window_->bounds().size());
  frame_generator_->SetDeviceScaleFactor(metrics_.device_scale_factor);
}

void PlatformDisplayDefault::OnAcceleratedWidgetDestroying() {}

void PlatformDisplayDefault::OnAcceleratedWidgetDestroyed() {
}

void PlatformDisplayDefault::OnActivationChanged(bool active) {}

}  // namespace ws
}  // namespace ui
