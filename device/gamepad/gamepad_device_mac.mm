// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/gamepad/gamepad_device_mac.h"

#include "base/mac/foundation_util.h"
#include "base/mac/scoped_cftyperef.h"

#import <Foundation/Foundation.h>

namespace {

// http://www.usb.org/developers/hidpage
const uint32_t kGenericDesktopUsagePage = 0x01;
const uint32_t kGameControlsUsagePage = 0x05;
const uint32_t kButtonUsagePage = 0x09;
const uint32_t kJoystickUsageNumber = 0x04;
const uint32_t kGameUsageNumber = 0x05;
const uint32_t kMultiAxisUsageNumber = 0x08;
const uint32_t kAxisMinimumUsageNumber = 0x30;

const int kRumbleMagnitudeMax = 10000;

float NormalizeAxis(CFIndex value, CFIndex min, CFIndex max) {
  return (2.f * (value - min) / static_cast<float>(max - min)) - 1.f;
}

float NormalizeUInt8Axis(uint8_t value, uint8_t min, uint8_t max) {
  return (2.f * (value - min) / static_cast<float>(max - min)) - 1.f;
}

float NormalizeUInt16Axis(uint16_t value, uint16_t min, uint16_t max) {
  return (2.f * (value - min) / static_cast<float>(max - min)) - 1.f;
}

float NormalizeUInt32Axis(uint32_t value, uint32_t min, uint32_t max) {
  return (2.f * (value - min) / static_cast<float>(max - min)) - 1.f;
}

}  // namespace

namespace device {

GamepadDeviceMac::GamepadDeviceMac(int location_id,
                                   IOHIDDeviceRef device_ref,
                                   int vendor_id,
                                   int product_id)
    : location_id_(location_id),
      device_ref_(device_ref),
      ff_device_ref_(nullptr),
      ff_effect_ref_(nullptr) {
  std::fill(button_elements_, button_elements_ + Gamepad::kButtonsLengthCap,
            nullptr);
  std::fill(axis_elements_, axis_elements_ + Gamepad::kAxesLengthCap, nullptr);
  std::fill(axis_minimums_, axis_minimums_ + Gamepad::kAxesLengthCap, 0);
  std::fill(axis_maximums_, axis_maximums_ + Gamepad::kAxesLengthCap, 0);
  std::fill(axis_report_sizes_, axis_report_sizes_ + Gamepad::kAxesLengthCap,
            0);

  if (Dualshock4ControllerMac::IsDualshock4(vendor_id, product_id)) {
    dualshock4_ = std::make_unique<Dualshock4ControllerMac>(device_ref);
  } else if (device_ref) {
    ff_device_ref_ = CreateForceFeedbackDevice(device_ref);
    if (ff_device_ref_) {
      ff_effect_ref_ = CreateForceFeedbackEffect(ff_device_ref_, &ff_effect_,
                                                 &ff_custom_force_, force_data_,
                                                 axes_data_, direction_data_);
    }
  }
}

GamepadDeviceMac::~GamepadDeviceMac() = default;

void GamepadDeviceMac::DoShutdown() {
  if (ff_device_ref_) {
    if (ff_effect_ref_)
      FFDeviceReleaseEffect(ff_device_ref_, ff_effect_ref_);
    FFReleaseDevice(ff_device_ref_);
    ff_effect_ref_ = nullptr;
    ff_device_ref_ = nullptr;
  }
  if (dualshock4_) {
    dualshock4_->Shutdown();
    dualshock4_ = nullptr;
  }
}

// static
bool GamepadDeviceMac::CheckCollection(IOHIDElementRef element) {
  // Check that a parent collection of this element matches one of the usage
  // numbers that we are looking for.
  while ((element = IOHIDElementGetParent(element)) != NULL) {
    uint32_t usage_page = IOHIDElementGetUsagePage(element);
    uint32_t usage = IOHIDElementGetUsage(element);
    if (usage_page == kGenericDesktopUsagePage) {
      if (usage == kJoystickUsageNumber || usage == kGameUsageNumber ||
          usage == kMultiAxisUsageNumber) {
        return true;
      }
    }
  }
  return false;
}

bool GamepadDeviceMac::AddButtonsAndAxes(Gamepad* gamepad) {
  base::ScopedCFTypeRef<CFArrayRef> elements_cf(IOHIDDeviceCopyMatchingElements(
      device_ref_, nullptr, kIOHIDOptionsTypeNone));
  NSArray* elements = base::mac::CFToNSCast(elements_cf);
  DCHECK(elements);
  DCHECK(gamepad);
  gamepad->axes_length = 0;
  gamepad->buttons_length = 0;
  gamepad->timestamp = 0;
  memset(gamepad->axes, 0, sizeof(gamepad->axes));
  memset(gamepad->buttons, 0, sizeof(gamepad->buttons));

  bool mapped_all_axes = true;

  for (id elem in elements) {
    IOHIDElementRef element = reinterpret_cast<IOHIDElementRef>(elem);
    if (!CheckCollection(element))
      continue;

    uint32_t usage_page = IOHIDElementGetUsagePage(element);
    uint32_t usage = IOHIDElementGetUsage(element);
    if (IOHIDElementGetType(element) == kIOHIDElementTypeInput_Button &&
        usage_page == kButtonUsagePage) {
      uint32_t button_index = usage - 1;
      if (button_index < Gamepad::kButtonsLengthCap) {
        button_elements_[button_index] = element;
        gamepad->buttons_length =
            std::max(gamepad->buttons_length, button_index + 1);
      }
    } else if (IOHIDElementGetType(element) == kIOHIDElementTypeInput_Misc) {
      uint32_t axis_index = usage - kAxisMinimumUsageNumber;
      if (axis_index < Gamepad::kAxesLengthCap && !axis_elements_[axis_index]) {
        axis_elements_[axis_index] = element;
        gamepad->axes_length = std::max(gamepad->axes_length, axis_index + 1);
      } else {
        mapped_all_axes = false;
      }
    }
  }

  if (!mapped_all_axes) {
    // For axes whose usage puts them outside the standard axesLengthCap range.
    uint32_t next_index = 0;
    for (id elem in elements) {
      IOHIDElementRef element = reinterpret_cast<IOHIDElementRef>(elem);
      if (!CheckCollection(element))
        continue;

      uint32_t usage_page = IOHIDElementGetUsagePage(element);
      uint32_t usage = IOHIDElementGetUsage(element);
      if (IOHIDElementGetType(element) == kIOHIDElementTypeInput_Misc &&
          usage - kAxisMinimumUsageNumber >= Gamepad::kAxesLengthCap &&
          usage_page <= kGameControlsUsagePage) {
        for (; next_index < Gamepad::kAxesLengthCap; ++next_index) {
          if (axis_elements_[next_index] == NULL)
            break;
        }
        if (next_index < Gamepad::kAxesLengthCap) {
          axis_elements_[next_index] = element;
          gamepad->axes_length = std::max(gamepad->axes_length, next_index + 1);
        }
      }

      if (next_index >= Gamepad::kAxesLengthCap)
        break;
    }
  }

  for (uint32_t axis_index = 0; axis_index < gamepad->axes_length;
       ++axis_index) {
    IOHIDElementRef element = axis_elements_[axis_index];
    if (element != NULL) {
      CFIndex axis_min = IOHIDElementGetLogicalMin(element);
      CFIndex axis_max = IOHIDElementGetLogicalMax(element);

      // Some HID axes report a logical range of -1 to 0 signed, which must be
      // interpreted as 0 to -1 unsigned for correct normalization behavior.
      if (axis_min == -1 && axis_max == 0) {
        axis_max = -1;
        axis_min = 0;
      }

      axis_minimums_[axis_index] = axis_min;
      axis_maximums_[axis_index] = axis_max;
      axis_report_sizes_[axis_index] = IOHIDElementGetReportSize(element);
    }
  }
  return (gamepad->axes_length > 0 || gamepad->buttons_length > 0);
}

void GamepadDeviceMac::UpdateGamepadForValue(IOHIDValueRef value,
                                             Gamepad* gamepad) {
  DCHECK(gamepad);
  IOHIDElementRef element = IOHIDValueGetElement(value);
  uint32_t value_length = IOHIDValueGetLength(value);
  if (value_length > 4) {
    // Workaround for bizarre issue with PS3 controllers that try to return
    // massive (30+ byte) values and crash IOHIDValueGetIntegerValue
    return;
  }

  // Find and fill in the associated button event, if any.
  for (size_t i = 0; i < gamepad->buttons_length; ++i) {
    if (button_elements_[i] == element) {
      bool pressed = IOHIDValueGetIntegerValue(value);
      gamepad->buttons[i].pressed = pressed;
      gamepad->buttons[i].value = pressed ? 1.f : 0.f;
      gamepad->timestamp =
          std::max(gamepad->timestamp, IOHIDValueGetTimeStamp(value));
      return;
    }
  }

  // Find and fill in the associated axis event, if any.
  for (size_t i = 0; i < gamepad->axes_length; ++i) {
    if (axis_elements_[i] == element) {
      CFIndex axis_min = axis_minimums_[i];
      CFIndex axis_max = axis_maximums_[i];
      CFIndex axis_value = IOHIDValueGetIntegerValue(value);

      if (axis_min > axis_max) {
        // We'll need to interpret this axis as unsigned during normalization.
        switch (axis_report_sizes_[i]) {
          case 8:
            gamepad->axes[i] =
                NormalizeUInt8Axis(axis_value, axis_min, axis_max);
            break;
          case 16:
            gamepad->axes[i] =
                NormalizeUInt16Axis(axis_value, axis_min, axis_max);
            break;
          case 32:
            gamepad->axes[i] =
                NormalizeUInt32Axis(axis_value, axis_min, axis_max);
            break;
        }
      } else {
        gamepad->axes[i] = NormalizeAxis(axis_value, axis_min, axis_max);
      }

      gamepad->timestamp =
          std::max(gamepad->timestamp, IOHIDValueGetTimeStamp(value));
      return;
    }
  }
}

bool GamepadDeviceMac::SupportsVibration() {
  return dualshock4_ || ff_device_ref_;
}

void GamepadDeviceMac::SetVibration(double strong_magnitude,
                                    double weak_magnitude) {
  if (dualshock4_) {
    dualshock4_->SetVibration(strong_magnitude, weak_magnitude);
  } else if (ff_device_ref_) {
    FFCUSTOMFORCE* ff_custom_force =
        static_cast<FFCUSTOMFORCE*>(ff_effect_.lpvTypeSpecificParams);
    DCHECK(ff_custom_force);
    DCHECK(ff_custom_force->rglForceData);

    ff_custom_force->rglForceData[0] =
        static_cast<LONG>(strong_magnitude * kRumbleMagnitudeMax);
    ff_custom_force->rglForceData[1] =
        static_cast<LONG>(weak_magnitude * kRumbleMagnitudeMax);

    // Download the effect to the device and start the effect.
    HRESULT res = FFEffectSetParameters(
        ff_effect_ref_, &ff_effect_,
        FFEP_DURATION | FFEP_STARTDELAY | FFEP_TYPESPECIFICPARAMS);
    if (res == FF_OK)
      FFEffectStart(ff_effect_ref_, 1, FFES_SOLO);
  }
}

void GamepadDeviceMac::SetZeroVibration() {
  if (dualshock4_) {
    dualshock4_->SetZeroVibration();
  } else if (ff_effect_ref_) {
    FFEffectStop(ff_effect_ref_);
  }
}

// static
FFDeviceObjectReference GamepadDeviceMac::CreateForceFeedbackDevice(
    IOHIDDeviceRef device_ref) {
  io_service_t service = IOHIDDeviceGetService(device_ref);

  if (service == MACH_PORT_NULL)
    return nullptr;

  HRESULT res = FFIsForceFeedback(service);
  if (res != FF_OK)
    return nullptr;

  FFDeviceObjectReference ff_device_ref;
  res = FFCreateDevice(service, &ff_device_ref);
  if (res != FF_OK)
    return nullptr;

  return ff_device_ref;
}

// static
FFEffectObjectReference GamepadDeviceMac::CreateForceFeedbackEffect(
    FFDeviceObjectReference ff_device_ref,
    FFEFFECT* ff_effect,
    FFCUSTOMFORCE* ff_custom_force,
    LONG* force_data,
    DWORD* axes_data,
    LONG* direction_data) {
  DCHECK(ff_effect);
  DCHECK(ff_custom_force);
  DCHECK(force_data);
  DCHECK(axes_data);
  DCHECK(direction_data);

  FFCAPABILITIES caps;
  HRESULT res = FFDeviceGetForceFeedbackCapabilities(ff_device_ref, &caps);
  if (res != FF_OK)
    return nullptr;

  if ((caps.supportedEffects & FFCAP_ET_CUSTOMFORCE) == 0)
    return nullptr;

  force_data[0] = 0;
  force_data[1] = 0;
  axes_data[0] = caps.ffAxes[0];
  axes_data[1] = caps.ffAxes[1];
  direction_data[0] = 0;
  direction_data[1] = 0;
  ff_custom_force->cChannels = 2;
  ff_custom_force->cSamples = 2;
  ff_custom_force->rglForceData = force_data;
  ff_custom_force->dwSamplePeriod = 100000;  // 100 ms
  ff_effect->dwSize = sizeof(FFEFFECT);
  ff_effect->dwFlags = FFEFF_OBJECTOFFSETS | FFEFF_SPHERICAL;
  ff_effect->dwDuration = 5000000;     // 5 seconds
  ff_effect->dwSamplePeriod = 100000;  // 100 ms
  ff_effect->dwGain = 10000;
  ff_effect->dwTriggerButton = FFEB_NOTRIGGER;
  ff_effect->dwTriggerRepeatInterval = 0;
  ff_effect->cAxes = caps.numFfAxes;
  ff_effect->rgdwAxes = axes_data;
  ff_effect->rglDirection = direction_data;
  ff_effect->lpEnvelope = nullptr;
  ff_effect->cbTypeSpecificParams = sizeof(FFCUSTOMFORCE);
  ff_effect->lpvTypeSpecificParams = ff_custom_force;
  ff_effect->dwStartDelay = 0;

  FFEffectObjectReference ff_effect_ref;
  res = FFDeviceCreateEffect(ff_device_ref, kFFEffectType_CustomForce_ID,
                             ff_effect, &ff_effect_ref);
  if (res != FF_OK)
    return nullptr;

  return ff_effect_ref;
}

}  // namespace device
