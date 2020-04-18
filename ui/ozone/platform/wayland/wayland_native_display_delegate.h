// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_WAYLAND_WAYLAND_NATIVE_DISPLAY_DELEGATE_H_
#define UI_OZONE_PLATFORM_WAYLAND_WAYLAND_NATIVE_DISPLAY_DELEGATE_H_

#include <stdint.h>

#include "base/macros.h"
#include "base/observer_list.h"
#include "ui/display/types/native_display_delegate.h"
#include "ui/ozone/platform/wayland/wayland_output.h"

namespace ui {

class WaylandConnection;

class WaylandNativeDisplayDelegate : public display::NativeDisplayDelegate,
                                     public WaylandOutput::Observer {
 public:
  explicit WaylandNativeDisplayDelegate(WaylandConnection* connection);
  ~WaylandNativeDisplayDelegate() override;

  // display::NativeDisplayDelegate overrides:
  void Initialize() override;
  void TakeDisplayControl(display::DisplayControlCallback callback) override;
  void RelinquishDisplayControl(
      display::DisplayControlCallback callback) override;
  void GetDisplays(display::GetDisplaysCallback callback) override;
  void Configure(const display::DisplaySnapshot& output,
                 const display::DisplayMode* mode,
                 const gfx::Point& origin,
                 display::ConfigureCallback callback) override;
  void GetHDCPState(const display::DisplaySnapshot& output,
                    display::GetHDCPStateCallback callback) override;
  void SetHDCPState(const display::DisplaySnapshot& output,
                    display::HDCPState state,
                    display::SetHDCPStateCallback callback) override;
  bool SetColorCorrection(
      const display::DisplaySnapshot& output,
      const std::vector<display::GammaRampRGBEntry>& degamma_lut,
      const std::vector<display::GammaRampRGBEntry>& gamma_lut,
      const std::vector<float>& correction_matrix) override;

  void AddObserver(display::NativeDisplayObserver* observer) override;
  void RemoveObserver(display::NativeDisplayObserver* observer) override;
  display::FakeDisplayController* GetFakeDisplayController() override;

  // WaylandOutput::Observer overrides:
  void OnOutputReadyForUse() override;

 private:
  WaylandConnection* connection_;  // Not owned.

  base::ObserverList<display::NativeDisplayObserver> observers_;

  bool displays_ready_ = false;

  DISALLOW_COPY_AND_ASSIGN(WaylandNativeDisplayDelegate);
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_WAYLAND_WAYLAND_NATIVE_DISPLAY_DELEGATE_H_
