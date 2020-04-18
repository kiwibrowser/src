// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/host/ash_window_tree_host_mus.h"

#include <memory>

#include "ash/host/root_window_transformer.h"
#include "ash/host/transformer_helper.h"
#include "ash/shell.h"
#include "ash/shell_delegate.h"
#include "services/ui/public/cpp/input_devices/input_device_controller_client.h"
#include "ui/aura/mus/window_tree_host_mus_init_params.h"
#include "ui/aura/window.h"
#include "ui/events/event_sink.h"
#include "ui/events/null_event_targeter.h"
#include "ui/gfx/geometry/rect_conversions.h"
#include "ui/gfx/geometry/rect_f.h"

namespace ash {

AshWindowTreeHostMus::AshWindowTreeHostMus(
    aura::WindowTreeHostMusInitParams init_params)
    : aura::WindowTreeHostMus(std::move(init_params)),
      transformer_helper_(std::make_unique<TransformerHelper>(this)) {
  transformer_helper_->Init();
}

AshWindowTreeHostMus::~AshWindowTreeHostMus() = default;

void AshWindowTreeHostMus::ConfineCursorToRootWindow() {
  if (!allow_confine_cursor())
    return;

  gfx::Rect confined_bounds(GetBoundsInPixels().size());
  confined_bounds.Inset(transformer_helper_->GetHostInsets());
  last_cursor_confine_bounds_in_pixels_ = confined_bounds;
  ConfineCursorToBounds(confined_bounds);
}

void AshWindowTreeHostMus::ConfineCursorToBoundsInRoot(
    const gfx::Rect& bounds_in_root) {
  if (!allow_confine_cursor())
    return;

  gfx::RectF bounds_f(bounds_in_root);
  GetRootTransform().TransformRect(&bounds_f);
  last_cursor_confine_bounds_in_pixels_ = gfx::ToEnclosingRect(bounds_f);
  ConfineCursorToBounds(last_cursor_confine_bounds_in_pixels_);
}

gfx::Rect AshWindowTreeHostMus::GetLastCursorConfineBoundsInPixels() const {
  return last_cursor_confine_bounds_in_pixels_;
}

void AshWindowTreeHostMus::SetRootWindowTransformer(
    std::unique_ptr<RootWindowTransformer> transformer) {
  transformer_helper_->SetRootWindowTransformer(std::move(transformer));
  ConfineCursorToRootWindow();
}

gfx::Insets AshWindowTreeHostMus::GetHostInsets() const {
  return transformer_helper_->GetHostInsets();
}

aura::WindowTreeHost* AshWindowTreeHostMus::AsWindowTreeHost() {
  return this;
}

void AshWindowTreeHostMus::PrepareForShutdown() {
  // WindowEventDispatcher may have pending events that need to be processed.
  // At the time this function is called the WindowTreeHost and Window are in
  // a semi-shutdown state. Reset the targeter so that the current targeter
  // doesn't attempt to process events while in this state, which would likely
  // crash.
  std::unique_ptr<ui::NullEventTargeter> null_event_targeter =
      std::make_unique<ui::NullEventTargeter>();
  window()->SetEventTargeter(std::move(null_event_targeter));

  // Mus will destroy the platform display/window and its accelerated widget;
  // prevent the compositor from using the asynchronously destroyed surface.
  DestroyCompositor();
}

void AshWindowTreeHostMus::RegisterMirroringHost(
    AshWindowTreeHost* mirroring_ash_host) {
  // This should not be called, but it is because mirroring isn't wired up for
  // mus. Once that is done, this should be converted to a NOTREACHED.
  NOTIMPLEMENTED_LOG_ONCE();
}

void AshWindowTreeHostMus::SetCursorConfig(
    const display::Display& display,
    display::Display::Rotation rotation) {
  // Nothing to do here, mus takes care of this.
}

void AshWindowTreeHostMus::ClearCursorConfig() {
  // Nothing to do here, mus takes care of this.
}

void AshWindowTreeHostMus::SetRootTransform(const gfx::Transform& transform) {
  transformer_helper_->SetTransform(transform);
}

gfx::Transform AshWindowTreeHostMus::GetRootTransform() const {
  return transformer_helper_->GetTransform();
}

gfx::Transform AshWindowTreeHostMus::GetInverseRootTransform() const {
  return transformer_helper_->GetInverseTransform();
}

gfx::Rect AshWindowTreeHostMus::GetTransformedRootWindowBoundsInPixels(
    const gfx::Size& host_size_in_pixels) const {
  return transformer_helper_->GetTransformedWindowBounds(host_size_in_pixels);
}

void AshWindowTreeHostMus::OnCursorVisibilityChangedNative(bool show) {
  ui::InputDeviceControllerClient* input_device_controller_client =
      Shell::Get()->shell_delegate()->GetInputDeviceControllerClient();
  if (!input_device_controller_client)
    return;  // Happens in tests.

  // Temporarily pause tap-to-click when the cursor is hidden.
  input_device_controller_client->SetTapToClickPaused(!show);
}

}  // namespace ash
