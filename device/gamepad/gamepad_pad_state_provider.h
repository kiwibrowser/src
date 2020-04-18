// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_GAMEPAD_GAMEPAD_PAD_STATE_PROVIDER_H_
#define DEVICE_GAMEPAD_GAMEPAD_PAD_STATE_PROVIDER_H_

#include <stdint.h>

#include <limits>
#include <memory>

#include "device/gamepad/gamepad_export.h"
#include "device/gamepad/gamepad_standard_mappings.h"
#include "device/gamepad/public/cpp/gamepad.h"

namespace device {

class GamepadDataFetcher;

enum GamepadSource {
  GAMEPAD_SOURCE_NONE = 0,
  GAMEPAD_SOURCE_ANDROID,
  GAMEPAD_SOURCE_GVR,
  GAMEPAD_SOURCE_CARDBOARD,
  GAMEPAD_SOURCE_LINUX_UDEV,
  GAMEPAD_SOURCE_MAC_GC,
  GAMEPAD_SOURCE_MAC_HID,
  GAMEPAD_SOURCE_MAC_XBOX,
  GAMEPAD_SOURCE_OCULUS,
  GAMEPAD_SOURCE_OPENVR,
  GAMEPAD_SOURCE_TEST,
  GAMEPAD_SOURCE_WIN_XINPUT,
  GAMEPAD_SOURCE_WIN_RAW,
};

struct PadState {
  // Which data fetcher provided this gamepad's data.
  GamepadSource source;
  // Data fetcher-specific identifier for this gamepad.
  int source_id;

  // Indicates whether this gamepad is actively receiving input. |is_active| is
  // initialized to false on each polling cycle and must is set to true when new
  // data is received.
  bool is_active;

  // True if the gamepad is newly connected but notifications have not yet been
  // sent.
  bool is_newly_active;

  // Set by the data fetcher to indicate that one-time initialization for this
  // gamepad has been completed.
  bool is_initialized;

  // Gamepad data, unmapped.
  Gamepad data;

  // Functions to map from device data to standard layout, if available. May
  // be null if no mapping is available or needed.
  GamepadStandardMappingFunction mapper;

  // Sanitization masks
  // axis_mask and button_mask are bitfields that represent the reset state of
  // each input. If a button or axis has ever reported 0 in the past the
  // corresponding bit will be set to 1.

  // If we ever increase the max axis count this will need to be updated.
  static_assert(Gamepad::kAxesLengthCap <=
                    std::numeric_limits<uint32_t>::digits,
                "axis_mask is not large enough");
  uint32_t axis_mask;

  // If we ever increase the max button count this will need to be updated.
  static_assert(Gamepad::kButtonsLengthCap <=
                    std::numeric_limits<uint32_t>::digits,
                "button_mask is not large enough");
  uint32_t button_mask;
};

class DEVICE_GAMEPAD_EXPORT GamepadPadStateProvider {
 public:
  GamepadPadStateProvider();
  virtual ~GamepadPadStateProvider();

  // Gets a PadState object for the given source and id. If the device hasn't
  // been encountered before one of the remaining slots will be reserved for it.
  // If no slots are available will return NULL.
  PadState* GetPadState(GamepadSource source, int source_id);

  // Gets a PadState object for a connected gamepad by specifying its index in
  // the pad_states_ array. Returns NULL if there is no connected gamepad at
  // that index.
  PadState* GetConnectedPadState(int pad_index);

 protected:
  void ClearPadState(PadState& state);

  void InitializeDataFetcher(GamepadDataFetcher* fetcher);

  void MapAndSanitizeGamepadData(PadState* pad_state,
                                 Gamepad* pad,
                                 bool sanitize);

  // Tracks the state of each gamepad slot.
  std::unique_ptr<PadState[]> pad_states_;
};

}  // namespace device

#endif  // DEVICE_GAMEPAD_GAMEPAD_PAD_STATE_PROVIDER_H_
