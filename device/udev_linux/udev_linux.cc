// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/udev_linux/udev_linux.h"

#include <stddef.h>

#include "base/bind.h"

namespace device {

UdevLinux::UdevLinux(const std::vector<UdevMonitorFilter>& filters,
                     const UdevNotificationCallback& callback)
    : udev_(udev_new()),
      monitor_(udev_ ? udev_monitor_new_from_netlink(udev_.get(), "udev")
                     : nullptr),
      monitor_fd_(-1),
      callback_(callback) {
  if (!monitor_) {
    LOG(ERROR) << "Failed to initialize udev, possibly due to an invalid "
               << "system configuration. Various device-related browser "
               << "features may be broken.";
    return;
  }

  for (const UdevMonitorFilter& filter : filters) {
    const int ret = udev_monitor_filter_add_match_subsystem_devtype(
        monitor_.get(), filter.subsystem, filter.devtype);
    CHECK_EQ(0, ret);
  }

  const int ret = udev_monitor_enable_receiving(monitor_.get());
  CHECK_EQ(0, ret);
  monitor_fd_ = udev_monitor_get_fd(monitor_.get());
  CHECK_GE(monitor_fd_, 0);

  monitor_watch_controller_ = base::FileDescriptorWatcher::WatchReadable(
      monitor_fd_, base::Bind(&UdevLinux::OnMonitorCanReadWithoutBlocking,
                              base::Unretained(this)));
}

UdevLinux::~UdevLinux() = default;

udev* UdevLinux::udev_handle() {
  return udev_.get();
}

void UdevLinux::OnMonitorCanReadWithoutBlocking() {
  // Events occur when devices attached to the system are added, removed, or
  // change state. udev_monitor_receive_device() will return a device object
  // representing the device which changed and what type of change occured.
  ScopedUdevDevicePtr dev(udev_monitor_receive_device(monitor_.get()));
  if (!dev)
    return;

  callback_.Run(dev.get());
}

}  // namespace device
