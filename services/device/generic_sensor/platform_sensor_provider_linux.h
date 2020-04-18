// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_DEVICE_GENERIC_SENSOR_PUBLIC_PLATFORM_SENSOR_PROVIDER_LINUX_H_
#define SERVICES_DEVICE_GENERIC_SENSOR_PUBLIC_PLATFORM_SENSOR_PROVIDER_LINUX_H_

#include "services/device/generic_sensor/platform_sensor_provider.h"

#include "base/single_thread_task_runner.h"
#include "services/device/generic_sensor/linux/sensor_device_manager.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
class Thread;
}  // namespace base

namespace device {

struct SensorInfoLinux;

class PlatformSensorProviderLinux : public PlatformSensorProvider,
                                    public SensorDeviceManager::Delegate {
 public:
  static PlatformSensorProviderLinux* GetInstance();

  // Sets another service provided by tests.
  void SetSensorDeviceManagerForTesting(
      std::unique_ptr<SensorDeviceManager> sensor_device_manager);

  // Sets task runner for tests.
  void SetFileTaskRunnerForTesting(
      scoped_refptr<base::SingleThreadTaskRunner> task_runner);

 protected:
  ~PlatformSensorProviderLinux() override;

  void CreateSensorInternal(mojom::SensorType type,
                            SensorReadingSharedBuffer* reading_buffer,
                            const CreateSensorCallback& callback) override;

  void FreeResources() override;

  void SetFileTaskRunner(
      scoped_refptr<base::SingleThreadTaskRunner> file_task_runner) override;

 private:
  friend struct base::DefaultSingletonTraits<PlatformSensorProviderLinux>;

  using SensorDeviceMap =
      std::unordered_map<mojom::SensorType, std::unique_ptr<SensorInfoLinux>>;

  PlatformSensorProviderLinux();

  void SensorDeviceFound(
      mojom::SensorType type,
      SensorReadingSharedBuffer* reading_buffer,
      const PlatformSensorProviderBase::CreateSensorCallback& callback,
      const SensorInfoLinux* sensor_device);

  bool StartPollingThread();

  // Stops a polling thread if there are no sensors left. Must be called on
  // a different than the polling thread which allows I/O.
  void StopPollingThread();

  // Shuts down a service that tracks events from iio subsystem.
  void Shutdown();

  // Returns SensorInfoLinux structure of a requested type.
  // If a request cannot be processed immediately, returns nullptr and
  // all the requests stored in |requests_map_| are processed after
  // enumeration is ready.
  SensorInfoLinux* GetSensorDevice(mojom::SensorType type);

  // Returns all found iio devices. Currently not implemented.
  void GetAllSensorDevices();

  // Processed stored requests in |request_map_|.
  void ProcessStoredRequests();

  // Called when sensors are created asynchronously after enumeration is done.
  void CreateSensorAndNotify(mojom::SensorType type,
                             SensorInfoLinux* sensor_device);

  // SensorDeviceManager::Delegate implements:
  void OnSensorNodesEnumerated() override;
  void OnDeviceAdded(mojom::SensorType type,
                     std::unique_ptr<SensorInfoLinux> sensor_device) override;
  void OnDeviceRemoved(mojom::SensorType type,
                       const std::string& device_node) override;

  void CreateFusionSensor(mojom::SensorType type,
                          SensorReadingSharedBuffer* reading_buffer,
                          const CreateSensorCallback& callback);

  // Set to true when enumeration is ready.
  bool sensor_nodes_enumerated_;

  // Set to true when |sensor_device_manager_| has already started enumeration.
  bool sensor_nodes_enumeration_started_;

  // Stores all available sensor devices by type.
  SensorDeviceMap sensor_devices_by_type_;

  // A thread that is used by sensor readers in case of polling strategy.
  std::unique_ptr<base::Thread> polling_thread_;

  // This manager is being used to get |SensorInfoLinux|, which represents
  // all the information of a concrete sensor provided by OS.
  std::unique_ptr<SensorDeviceManager> sensor_device_manager_;

  // Browser's file thread task runner passed from renderer. Used by this
  // provider to stop a polling thread and passed to a manager that
  // runs a linux device monitor service on this task runner.
  scoped_refptr<base::SingleThreadTaskRunner> file_task_runner_;

  DISALLOW_COPY_AND_ASSIGN(PlatformSensorProviderLinux);
};

}  // namespace device

#endif  // SERVICES_DEVICE_GENERIC_SENSOR_PUBLIC_PLATFORM_SENSOR_PROVIDER_LINUX_H_
