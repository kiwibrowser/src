// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_GAMEPAD_RAW_INPUT_GAMEPAD_DEVICE_WIN_
#define DEVICE_GAMEPAD_RAW_INPUT_GAMEPAD_DEVICE_WIN_

#include <Unknwn.h>
#include <WinDef.h>
#include <hidsdi.h>
#include <stdint.h>
#include <stdlib.h>
#include <windows.h>

#include <memory>

#include "device/gamepad/abstract_haptic_gamepad.h"
#include "device/gamepad/dualshock4_controller_win.h"
#include "device/gamepad/hid_dll_functions_win.h"
#include "device/gamepad/public/cpp/gamepad.h"

namespace device {

class RawInputGamepadDeviceWin : public AbstractHapticGamepad {
 public:
  // Relevant usage IDs within the Generic Desktop usage page. RawInput gamepads
  // must have one of these usage IDs.
  static const uint16_t kGenericDesktopJoystick = 0x04;
  static const uint16_t kGenericDesktopGamePad = 0x05;
  static const uint16_t kGenericDesktopMultiAxisController = 0x08;

  RawInputGamepadDeviceWin(HANDLE device_handle,
                           int source_id,
                           HidDllFunctionsWin* hid_functions);
  ~RawInputGamepadDeviceWin() override;

  static bool IsGamepadUsageId(uint16_t usage);

  int GetSourceId() const { return source_id_; }
  int GetVendorId() const { return vendor_id_; }
  int GetVersionNumber() const { return version_number_; }
  int GetProductId() const { return product_id_; }
  std::wstring GetDeviceName() const { return name_; }
  std::wstring GetProductString() const { return product_string_; }

  // Return true if this device is a gamepad.
  bool IsValid() const { return is_valid_; }

  // Return true if this device supports vibration effects.
  bool SupportsVibration() const;

  // Updates the current gamepad state with data from a RAWINPUT event.
  void UpdateGamepad(RAWINPUT* input);

  // Read the current gamepad state into |pad|.
  void ReadPadState(Gamepad* pad) const;

  // Set the vibration magnitude for the strong and weak vibration actuators.
  void SetVibration(double strong_magnitude, double weak_magnitude) override;

 private:
  // Axis state and capabilities for a single RawInput axis.
  struct RawGamepadAxis {
    HIDP_VALUE_CAPS caps;
    float value;
    bool active;
    unsigned long bitmask;
  };

  // Stop vibration and release held resources.
  void DoShutdown() override;

  // Fetch information about this device. Returns true if the device appears to
  // be a valid gamepad.
  bool QueryDeviceInfo();

  // Fetch HID properties (RID_DEVICE_INFO_HID). Returns false on failure.
  bool QueryHidInfo();

  // Fetch the device name (RIDI_DEVICENAME). Returns false on failure.
  bool QueryDeviceName();

  // Fetch the product string. Returns false if none is available.
  bool QueryProductString();

  // These methods fetch information about the capabilities of buttons and axes
  // on the device.
  bool QueryDeviceCapabilities();
  void QueryButtonCapabilities(uint16_t button_count);
  void QueryAxisCapabilities(uint16_t axis_count);

  // True if the device described by this object is a valid RawInput gamepad.
  bool is_valid_ = false;

  // The device handle.
  HANDLE handle_ = nullptr;

  // The index assigned to this gamepad by the data fetcher.
  int source_id_ = 0;

  // Functions loaded from hid.dll. Not owned.
  HidDllFunctionsWin* hid_functions_ = nullptr;

  // The report ID incremented each time an input message is received for this
  // device. It is included in the pad info in place of a timestamp.
  uint32_t report_id_ = 0;

  uint32_t vendor_id_ = 0;
  uint32_t product_id_ = 0;
  uint32_t version_number_ = 0;
  uint16_t usage_ = 0;
  std::wstring name_;
  std::wstring product_string_;

  size_t buttons_length_ = 0;
  bool buttons_[Gamepad::kButtonsLengthCap];

  size_t axes_length_ = 0;
  RawGamepadAxis axes_[Gamepad::kAxesLengthCap];

  // Buffer used for querying device capabilities. |ppd_buffer_| owns the
  // memory pointed to by |preparsed_data_|.
  std::unique_ptr<uint8_t[]> ppd_buffer_;
  PHIDP_PREPARSED_DATA preparsed_data_ = nullptr;

  // Dualshock4-specific functionality (e.g., haptics), if available.
  std::unique_ptr<Dualshock4ControllerWin> dualshock4_;
};

}  // namespace device

#endif  // DEVICE_GAMEPAD_RAW_INPUT_GAMEPAD_DEVICE_WIN_
