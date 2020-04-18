// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_DEVICE_GENERIC_SENSOR_LINUX_SENSOR_DEVICE_MANAGER_H_
#define SERVICES_DEVICE_GENERIC_SENSOR_LINUX_SENSOR_DEVICE_MANAGER_H_

#include "base/scoped_observer.h"
#include "base/single_thread_task_runner.h"
#include "device/base/device_monitor_linux.h"
#include "services/device/public/mojom/sensor.mojom.h"

namespace device {

struct SensorInfoLinux;

// This SensorDeviceManager uses LinuxDeviceMonitor to enumerate devices
// and listen to "add/removed" events to notify |provider_| about
// added or removed iio devices. It has own cache to speed up an identification
// process of removed devices.
class SensorDeviceManager : public DeviceMonitorLinux::Observer {
 public:
  class Delegate {
   public:
    // Called when SensorDeviceManager has enumerated through all possible
    // iio udev devices.
    virtual void OnSensorNodesEnumerated() = 0;

    // Called after SensorDeviceManager has identified a udev device, which
    // belongs to "iio" subsystem.
    virtual void OnDeviceAdded(mojom::SensorType type,
                               std::unique_ptr<SensorInfoLinux> sensor) = 0;

    // Called after "removed" event is received from LinuxDeviceMonitor and
    // sensor is identified as known.
    virtual void OnDeviceRemoved(mojom::SensorType type,
                                 const std::string& device_node) = 0;

   protected:
    virtual ~Delegate() {}
  };

  SensorDeviceManager();
  ~SensorDeviceManager() override;

  // Starts this service.
  virtual void Start(Delegate* delegate);

 protected:
  using SensorDeviceMap = std::unordered_map<std::string, mojom::SensorType>;

  // Wrappers around udev system methods that can be implemented differently
  // by tests.
  virtual std::string GetUdevDeviceGetSubsystem(udev_device* dev);
  virtual std::string GetUdevDeviceGetSyspath(udev_device* dev);
  virtual std::string GetUdevDeviceGetSysattrValue(
      udev_device* dev,
      const std::string& attribute);
  virtual std::string GetUdevDeviceGetDevnode(udev_device* dev);

  // DeviceMonitorLinux::Observer:
  void OnDeviceAdded(udev_device* udev_device) override;
  void OnDeviceRemoved(udev_device* device) override;

  // Represents a map of sensors that are already known to this manager after
  // initial enumeration.
  SensorDeviceMap sensors_by_node_;

  THREAD_CHECKER(thread_checker_);

  ScopedObserver<DeviceMonitorLinux, DeviceMonitorLinux::Observer> observer_;

  Delegate* delegate_;

  // A task runner, which |delegate_| lives on.
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  DISALLOW_COPY_AND_ASSIGN(SensorDeviceManager);
};

}  // namespace device

#endif  // SERVICES_DEVICE_GENERIC_SENSOR_LINUX_SENSOR_DEVICE_MANAGER_H_
