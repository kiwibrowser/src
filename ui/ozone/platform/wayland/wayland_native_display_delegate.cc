// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/wayland/wayland_native_display_delegate.h"

#include "ui/display/types/display_snapshot.h"
#include "ui/display/types/native_display_observer.h"
#include "ui/ozone/platform/wayland/wayland_connection.h"

namespace ui {

WaylandNativeDisplayDelegate::WaylandNativeDisplayDelegate(
    WaylandConnection* connection)
    : connection_(connection) {}

WaylandNativeDisplayDelegate::~WaylandNativeDisplayDelegate() {
  connection_->PrimaryOutput()->SetObserver(nullptr);
}

void WaylandNativeDisplayDelegate::Initialize() {
  // TODO(msisov): Add support for secondary output.
  WaylandOutput* primary_output = connection_->PrimaryOutput();
  if (!primary_output)
    NOTREACHED() << "Asynchronous display data fetching is not available";

  primary_output->SetObserver(this);
}

void WaylandNativeDisplayDelegate::TakeDisplayControl(
    display::DisplayControlCallback callback) {
  NOTREACHED();
}

void WaylandNativeDisplayDelegate::RelinquishDisplayControl(
    display::DisplayControlCallback callback) {
  NOTREACHED();
}

void WaylandNativeDisplayDelegate::GetDisplays(
    display::GetDisplaysCallback callback) {
  if (displays_ready_)
    connection_->PrimaryOutput()->GetDisplaysSnapshot(std::move(callback));
}

void WaylandNativeDisplayDelegate::Configure(
    const display::DisplaySnapshot& output,
    const display::DisplayMode* mode,
    const gfx::Point& origin,
    display::ConfigureCallback callback) {
  NOTREACHED();
}

void WaylandNativeDisplayDelegate::GetHDCPState(
    const display::DisplaySnapshot& output,
    display::GetHDCPStateCallback callback) {
  NOTREACHED();
}

void WaylandNativeDisplayDelegate::SetHDCPState(
    const display::DisplaySnapshot& output,
    display::HDCPState state,
    display::SetHDCPStateCallback callback) {
  NOTREACHED();
}

bool WaylandNativeDisplayDelegate::SetColorCorrection(
    const display::DisplaySnapshot& output,
    const std::vector<display::GammaRampRGBEntry>& degamma_lut,
    const std::vector<display::GammaRampRGBEntry>& gamma_lut,
    const std::vector<float>& correction_matrix) {
  NOTREACHED();
  return false;
}

void WaylandNativeDisplayDelegate::AddObserver(
    display::NativeDisplayObserver* observer) {
  observers_.AddObserver(observer);
}

void WaylandNativeDisplayDelegate::RemoveObserver(
    display::NativeDisplayObserver* observer) {
  observers_.RemoveObserver(observer);
}

display::FakeDisplayController*
WaylandNativeDisplayDelegate::GetFakeDisplayController() {
  return nullptr;
}

void WaylandNativeDisplayDelegate::OnOutputReadyForUse() {
  if (!displays_ready_)
    displays_ready_ = true;

  for (display::NativeDisplayObserver& observer : observers_)
    observer.OnConfigurationChanged();
}

}  // namespace ui
