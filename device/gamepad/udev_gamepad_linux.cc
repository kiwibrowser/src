// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/gamepad/udev_gamepad_linux.h"

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "device/udev_linux/udev.h"

namespace device {

namespace {

int DeviceIndexFromDevicePath(const std::string& path,
                              const std::string& prefix) {
  if (!base::StartsWith(path, prefix, base::CompareCase::SENSITIVE))
    return -1;

  int index = -1;
  base::StringPiece index_str(&path.c_str()[prefix.length()],
                              path.length() - prefix.length());
  if (!base::StringToInt(index_str, &index))
    return -1;

  return index;
}

}  // namespace

const char UdevGamepadLinux::kInputSubsystem[] = "input";
const char UdevGamepadLinux::kHidrawSubsystem[] = "hidraw";

UdevGamepadLinux::UdevGamepadLinux(Type type,
                                   int index,
                                   const std::string& path,
                                   const std::string& syspath_prefix)
    : type(type), index(index), path(path), syspath_prefix(syspath_prefix) {}

// static
std::unique_ptr<UdevGamepadLinux> UdevGamepadLinux::Create(udev_device* dev) {
  using DeviceRootPair = std::pair<Type, const char*>;
  static const std::vector<DeviceRootPair> device_roots = {
      {Type::EVDEV, "/dev/input/event"},
      {Type::JOYDEV, "/dev/input/js"},
      {Type::HIDRAW, "/dev/hidraw"},
  };

  if (!dev)
    return nullptr;

  const char* node_path = device::udev_device_get_devnode(dev);
  if (!node_path)
    return nullptr;

  const char* node_syspath = device::udev_device_get_syspath(dev);

  udev_device* parent_dev =
      device::udev_device_get_parent_with_subsystem_devtype(
          dev, kInputSubsystem, nullptr);
  const char* parent_syspath =
      parent_dev ? device::udev_device_get_syspath(parent_dev) : "";

  for (const auto& entry : device_roots) {
    Type node_type = entry.first;
    const char* prefix = entry.second;
    int index_value = DeviceIndexFromDevicePath(node_path, prefix);

    if (index_value < 0)
      continue;

    // The syspath can be used to associate device nodes that describe the same
    // gamepad through multiple interfaces. For input nodes (evdev and joydev),
    // we use the syspath of the parent node, which describes the underlying
    // physical device. For hidraw nodes, we use the syspath of the node itself.
    //
    // The parent syspaths for matching evdev and joydev nodes will be identical
    // because they share the same parent node. The syspath for hidraw nodes
    // will also be identical up to the subsystem. For instance, if the syspath
    // for the input subsystem is:
    //     /sys/devices/[...]/0003:054C:09CC.0026/input/input91
    // And the corresponding hidraw syspath is:
    //     /sys/devices/[...]/0003:054C:09CC.0026/hidraw/hidraw3
    // Then |syspath_prefix| is the common prefix before "input" or "hidraw":
    //     /sys/devices/[...]/0003:054C:09CC.0026/
    std::string syspath;
    std::string subsystem;
    if (node_type == Type::EVDEV || node_type == Type::JOYDEV) {
      // If the device is in the input subsystem but does not have the
      // ID_INPUT_JOYSTICK property, ignore it.
      if (!device::udev_device_get_property_value(dev, "ID_INPUT_JOYSTICK"))
        return nullptr;

      syspath = parent_syspath;
      subsystem = kInputSubsystem;
    } else if (node_type == Type::HIDRAW) {
      syspath = node_syspath;
      subsystem = kHidrawSubsystem;
    }

    size_t subsystem_start = syspath.find(subsystem);
    if (subsystem_start == std::string::npos)
      return nullptr;
    std::string syspath_prefix = syspath.substr(0, subsystem_start);

    UdevGamepadLinux* pad_info =
        new UdevGamepadLinux(node_type, index_value, node_path, syspath_prefix);
    return std::unique_ptr<UdevGamepadLinux>(pad_info);
  }

  return nullptr;
}

}  // namespace device
