// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_GAMEPAD_UDEV_GAMEPAD_
#define DEVICE_GAMEPAD_UDEV_GAMEPAD_

#include <memory>
#include <string>

extern "C" {
struct udev_device;
}

namespace device {

class UdevGamepadLinux {
 public:
  enum class Type {
    JOYDEV,
    EVDEV,
    HIDRAW,
  };

  static const char kInputSubsystem[];
  static const char kHidrawSubsystem[];

  ~UdevGamepadLinux() = default;

  // Factory method for creating UdevGamepadLinux instances. Extracts info
  // about the device and returns a UdevGamepadLinux describing it, or nullptr
  // if the device cannot be a gamepad.
  static std::unique_ptr<UdevGamepadLinux> Create(udev_device* dev);

  const Type type;
  const int index;
  const std::string path;
  const std::string syspath_prefix;

 private:
  UdevGamepadLinux(Type type,
                   int index,
                   const std::string& path,
                   const std::string& syspath_prefix);
};

}  // namespace device

#endif  // DEVICE_GAMEPAD_UDEV_GAMEPAD_
