// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// UdevLinux listens for device change notifications from udev and runs
// callbacks when notifications occur.
//
// UdevLinux must be created on a thread that has instantiated a
// FileDescriptorWatcher. UdevLinux is not thread-safe.
//
// Example usage:
//
// class UdevLinux;
//
// class Foo {
//  public:
//   Foo() {
//     std::vector<UdevLinux::UdevMonitorFilter> filters;
//     filters.push_back(UdevLinux::UdevMonitorFilter("block", NULL));
//     udev_.reset(new UdevLinux(filters,
//                               base::Bind(&Foo::Notify, this)));
//   }
//
//   // Called when a "block" device attaches/detaches.
//   // To hold on to |device|, call udev_device_ref(device).
//   void Notify(udev_device* device) {
//     // Do something with |device|.
//   }
//
//  private:
//   std::unique_ptr<UdevLinux> udev_;
//
//   DISALLOW_COPY_AND_ASSIGN(Foo);
// };

#ifndef DEVICE_UDEV_LINUX_UDEV_LINUX_H_
#define DEVICE_UDEV_LINUX_UDEV_LINUX_H_

#include <memory>
#include <vector>

#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/files/file_descriptor_watcher_posix.h"
#include "base/macros.h"
#include "device/udev_linux/scoped_udev.h"

extern "C" {
struct udev;
struct udev_device;
}

namespace device {

class UdevLinux {
 public:
  typedef base::Callback<void(udev_device*)> UdevNotificationCallback;

  // subsystem and devtype parameter for
  // udev_monitor_filter_add_match_subsystem_devtype().
  struct UdevMonitorFilter {
    UdevMonitorFilter(const char* subsystem_in, const char* devtype_in)
        : subsystem(subsystem_in), devtype(devtype_in) {}
    const char* subsystem;
    const char* devtype;
  };

  // Filter incoming devices based on |filters|.
  // Calls |callback| upon device change events.
  UdevLinux(const std::vector<UdevMonitorFilter>& filters,
            const UdevNotificationCallback& callback);
  ~UdevLinux();

  // Returns the udev handle to be passed into other udev_*() functions.
  udev* udev_handle();

 private:
  // Called when |monitor_fd_| can be read without blocking.
  void OnMonitorCanReadWithoutBlocking();

  // libudev-related items, the main context, and the monitoring context to be
  // notified about changes to device states.
  const ScopedUdevPtr udev_;
  const ScopedUdevMonitorPtr monitor_;
  int monitor_fd_;
  std::unique_ptr<base::FileDescriptorWatcher::Controller>
      monitor_watch_controller_;
  const UdevNotificationCallback callback_;

  DISALLOW_COPY_AND_ASSIGN(UdevLinux);
};

}  // namespace device

#endif  // DEVICE_UDEV_LINUX_UDEV_LINUX_H_
