// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This class is used to detect device change and notify base::SystemMonitor
// on Linux.

#ifndef MEDIA_DEVICE_MONITORS_DEVICE_MONITOR_UDEV_H_
#define MEDIA_DEVICE_MONITORS_DEVICE_MONITOR_UDEV_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/message_loop/message_loop_current.h"
#include "base/single_thread_task_runner.h"
#include "media/base/media_export.h"

extern "C" {
struct udev_device;
}

namespace device {
class UdevLinux;
}

namespace media {

class MEDIA_EXPORT DeviceMonitorLinux
    : public base::MessageLoopCurrent::DestructionObserver {
 public:
  explicit DeviceMonitorLinux(
      const scoped_refptr<base::SingleThreadTaskRunner>& io_task_runner);
  ~DeviceMonitorLinux() override;

  // TODO(mcasas): Consider adding a StartMonitoring() method like
  // DeviceMonitorMac to reduce startup impact time.

 private:
  // This object is deleted on the constructor thread after |io_task_runner_|
  // has been destroyed. Need to know when the latter is being destroyed so that
  // we can delete |udev_|.
  void WillDestroyCurrentMessageLoop() override;

  void Initialize();
  void OnDevicesChanged(udev_device* device);

  const scoped_refptr<base::SingleThreadTaskRunner> io_task_runner_;

  std::unique_ptr<device::UdevLinux> udev_;

  DISALLOW_COPY_AND_ASSIGN(DeviceMonitorLinux);
};

}  // namespace media

#endif  // MEDIA_DEVICE_MONITORS_DEVICE_MONITOR_UDEV_H_
