// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/udev_linux/udev_watcher.h"

#include "base/bind.h"
#include "base/memory/ptr_util.h"

namespace device {

UdevWatcher::Observer::~Observer() = default;

void UdevWatcher::Observer::OnDeviceAdded(ScopedUdevDevicePtr device) {}

void UdevWatcher::Observer::OnDeviceRemoved(ScopedUdevDevicePtr device) {}

std::unique_ptr<UdevWatcher> UdevWatcher::StartWatching(Observer* observer) {
  ScopedUdevPtr udev(udev_new());
  if (!udev) {
    LOG(ERROR) << "Failed to initialize udev.";
    return nullptr;
  }

  ScopedUdevMonitorPtr udev_monitor(
      udev_monitor_new_from_netlink(udev.get(), "udev"));
  if (!udev_monitor) {
    LOG(ERROR) << "Failed to initialize a udev monitor.";
    return nullptr;
  }

  if (udev_monitor_enable_receiving(udev_monitor.get()) != 0) {
    LOG(ERROR) << "Failed to enable receiving udev events.";
    return nullptr;
  }

  int monitor_fd = udev_monitor_get_fd(udev_monitor.get());
  if (monitor_fd < 0) {
    LOG(ERROR) << "Udev monitor file descriptor unavailable.";
    return nullptr;
  }

  return base::WrapUnique(new UdevWatcher(
      std::move(udev), std::move(udev_monitor), monitor_fd, observer));
}

UdevWatcher::~UdevWatcher() {
  DCHECK(sequence_checker_.CalledOnValidSequence());
};

void UdevWatcher::EnumerateExistingDevices() {
  DCHECK(sequence_checker_.CalledOnValidSequence());
  ScopedUdevEnumeratePtr enumerate(udev_enumerate_new(udev_.get()));
  if (!enumerate) {
    LOG(ERROR) << "Failed to initialize a udev enumerator.";
    return;
  }

  if (udev_enumerate_scan_devices(enumerate.get()) != 0) {
    LOG(ERROR) << "Failed to begin udev enumeration.";
    return;
  }

  udev_list_entry* devices = udev_enumerate_get_list_entry(enumerate.get());
  for (udev_list_entry* i = devices; i != nullptr;
       i = udev_list_entry_get_next(i)) {
    ScopedUdevDevicePtr device(
        udev_device_new_from_syspath(udev_.get(), udev_list_entry_get_name(i)));
    if (device)
      observer_->OnDeviceAdded(std::move(device));
  }
}

UdevWatcher::UdevWatcher(ScopedUdevPtr udev,
                         ScopedUdevMonitorPtr udev_monitor,
                         int monitor_fd,
                         Observer* observer)
    : udev_(std::move(udev)),
      udev_monitor_(std::move(udev_monitor)),
      observer_(observer) {
  file_watcher_ = base::FileDescriptorWatcher::WatchReadable(
      monitor_fd,
      base::Bind(&UdevWatcher::OnMonitorReadable, base::Unretained(this)));
}

void UdevWatcher::OnMonitorReadable() {
  ScopedUdevDevicePtr device(udev_monitor_receive_device(udev_monitor_.get()));
  if (!device)
    return;

  std::string action(udev_device_get_action(device.get()));
  if (action == "add")
    observer_->OnDeviceAdded(std::move(device));
  else if (action == "remove")
    observer_->OnDeviceRemoved(std::move(device));
}

}  // namespace device
