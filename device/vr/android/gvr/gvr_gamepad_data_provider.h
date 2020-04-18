// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_VR_ANDROID_GVR_GAMEPAD_DATA_PROVIDER_H
#define DEVICE_VR_ANDROID_GVR_GAMEPAD_DATA_PROVIDER_H

#include "ui/gfx/geometry/quaternion.h"
#include "ui/gfx/geometry/vector2d_f.h"
#include "ui/gfx/geometry/vector3d_f.h"

namespace device {

class GvrGamepadDataFetcher;

// Subset of GVR controller data needed for the gamepad API. Filled in
// by vr_shell's VrController and consumed by GvrGamepadDataFetcher.
struct GvrGamepadData {
  GvrGamepadData()
      : timestamp(0),
        is_touching(false),
        controller_button_pressed(false),
        right_handed(true),
        connected(false) {}
  int64_t timestamp;
  gfx::Vector2dF touch_pos;
  gfx::Quaternion orientation;
  gfx::Vector3dF accel;
  gfx::Vector3dF gyro;
  bool is_touching;
  bool controller_button_pressed;
  bool right_handed;
  bool connected;
};

// This class exposes GVR controller data to the gamepad API. Data is
// polled by VrShellGl, then pushed from VrShell which implements the
// GvrGamepadDataProvider interface.
//
// More specifically, here's the lifecycle, assuming VrShell
// implements GvrGamepadDataProvider:
//
// - VrShell creates GvrGamepadDataFetcherFactory from
//   VrShell::UpdateGamepadData.
//
// - GvrGamepadDataFetcherFactory creates GvrGamepadDataFetcher.
//
// - GvrGamepadDataFetcher registers itself with VrShell via
//   VrShell::RegisterGamepadDataFetcher.
//
// - While presenting, VrShell::UpdateGamepadData calls
//   GvrGamepadDataFetcher->SetGamepadData to push poses,
//   GvrGamepadDataFetcher::GetGamepadData returns these when polled.
//
// - VrShell starts executing its destructor.
//
// - VrShell destructor unregisters GvrGamepadDataFetcherFactory.
//
// - GvrGamepadDataFetcherFactory destructor destroys GvrGamepadDataFetcher.
//
class GvrGamepadDataProvider {
 public:
  // Refresh current GVR controller data for use by gamepad API. The
  // implementation also lazily creates and registers the GVR
  // GamepadDataFetcherFactory with the GamepadDataFetcherManager as
  // needed.
  virtual void UpdateGamepadData(GvrGamepadData data) = 0;

  // Called by the gamepad data fetcher constructor to register itself
  // for receiving data via SetGamepadData. The fetcher must remain
  // alive while the provider is calling SetGamepadData on it.
  virtual void RegisterGvrGamepadDataFetcher(GvrGamepadDataFetcher*) = 0;
};

}  // namespace device
#endif  // DEVICE_VR_ANDROID_GVR_GAMEPAD_DATA_PROVIDER_H
