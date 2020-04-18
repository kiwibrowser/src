// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/device/generic_sensor/platform_sensor_provider_linux.h"

#include <utility>
#include <vector>

#include "base/memory/ref_counted.h"
#include "base/memory/singleton.h"
#include "base/message_loop/message_loop.h"
#include "base/task_runner_util.h"
#include "base/threading/thread.h"
#include "services/device/generic_sensor/absolute_orientation_euler_angles_fusion_algorithm_using_accelerometer_and_magnetometer.h"
#include "services/device/generic_sensor/linear_acceleration_fusion_algorithm_using_accelerometer.h"
#include "services/device/generic_sensor/linux/sensor_data_linux.h"
#include "services/device/generic_sensor/orientation_quaternion_fusion_algorithm_using_euler_angles.h"
#include "services/device/generic_sensor/platform_sensor_fusion.h"
#include "services/device/generic_sensor/platform_sensor_linux.h"
#include "services/device/generic_sensor/platform_sensor_reader_linux.h"
#include "services/device/generic_sensor/relative_orientation_euler_angles_fusion_algorithm_using_accelerometer.h"
#include "services/device/generic_sensor/relative_orientation_euler_angles_fusion_algorithm_using_accelerometer_and_gyroscope.h"

namespace device {
namespace {
bool IsFusionSensorType(mojom::SensorType type) {
  switch (type) {
    case mojom::SensorType::LINEAR_ACCELERATION:
    case mojom::SensorType::ABSOLUTE_ORIENTATION_EULER_ANGLES:
    case mojom::SensorType::ABSOLUTE_ORIENTATION_QUATERNION:
    case mojom::SensorType::RELATIVE_ORIENTATION_EULER_ANGLES:
    case mojom::SensorType::RELATIVE_ORIENTATION_QUATERNION:
      return true;
    default:
      return false;
  }
}
}  // namespace

// static
PlatformSensorProviderLinux* PlatformSensorProviderLinux::GetInstance() {
  return base::Singleton<
      PlatformSensorProviderLinux,
      base::LeakySingletonTraits<PlatformSensorProviderLinux>>::get();
}

PlatformSensorProviderLinux::PlatformSensorProviderLinux()
    : sensor_nodes_enumerated_(false),
      sensor_nodes_enumeration_started_(false),
      sensor_device_manager_(nullptr) {}

PlatformSensorProviderLinux::~PlatformSensorProviderLinux() {
  Shutdown();
}

void PlatformSensorProviderLinux::CreateSensorInternal(
    mojom::SensorType type,
    SensorReadingSharedBuffer* reading_buffer,
    const CreateSensorCallback& callback) {
  if (!sensor_device_manager_)
    sensor_device_manager_.reset(new SensorDeviceManager());

  if (!sensor_nodes_enumerated_) {
    if (!sensor_nodes_enumeration_started_) {
      sensor_nodes_enumeration_started_ = file_task_runner_->PostTask(
          FROM_HERE,
          base::Bind(&SensorDeviceManager::Start,
                     base::Unretained(sensor_device_manager_.get()), this));
    }
    return;
  }

  if (IsFusionSensorType(type)) {
    CreateFusionSensor(type, reading_buffer, callback);
    return;
  }

  SensorInfoLinux* sensor_device = GetSensorDevice(type);
  if (!sensor_device) {
    callback.Run(nullptr);
    return;
  }

  SensorDeviceFound(type, reading_buffer, callback, sensor_device);
}

void PlatformSensorProviderLinux::SensorDeviceFound(
    mojom::SensorType type,
    SensorReadingSharedBuffer* reading_buffer,
    const PlatformSensorProviderBase::CreateSensorCallback& callback,
    const SensorInfoLinux* sensor_device) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(sensor_device);

  if (!StartPollingThread()) {
    callback.Run(nullptr);
    return;
  }

  scoped_refptr<PlatformSensorLinux> sensor =
      new PlatformSensorLinux(type, reading_buffer, this, sensor_device,
                              polling_thread_->task_runner());
  callback.Run(sensor);
}

void PlatformSensorProviderLinux::SetFileTaskRunner(
    scoped_refptr<base::SingleThreadTaskRunner> file_task_runner) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (!file_task_runner_)
    file_task_runner_ = file_task_runner;
}

void PlatformSensorProviderLinux::FreeResources() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(file_task_runner_);
  // When there are no sensors left, the polling thread must be stopped.
  // Stop() can only be called on a different thread that allows I/O.
  // Thus, browser's file thread is used for this purpose.
  file_task_runner_->PostTask(
      FROM_HERE, base::Bind(&PlatformSensorProviderLinux::StopPollingThread,
                            base::Unretained(this)));
}

bool PlatformSensorProviderLinux::StartPollingThread() {
  if (!polling_thread_)
    polling_thread_.reset(new base::Thread("Sensor polling thread"));

  if (!polling_thread_->IsRunning()) {
    return polling_thread_->StartWithOptions(
        base::Thread::Options(base::MessageLoop::TYPE_IO, 0));
  }
  return true;
}

void PlatformSensorProviderLinux::StopPollingThread() {
  DCHECK(file_task_runner_);
  DCHECK(file_task_runner_->BelongsToCurrentThread());
  if (polling_thread_ && polling_thread_->IsRunning())
    polling_thread_->Stop();
}

void PlatformSensorProviderLinux::Shutdown() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  const bool did_post_task = file_task_runner_->DeleteSoon(
      FROM_HERE, sensor_device_manager_.release());
  DCHECK(did_post_task);
  sensor_nodes_enumerated_ = false;
  sensor_nodes_enumeration_started_ = false;
  sensor_devices_by_type_.clear();
}

SensorInfoLinux* PlatformSensorProviderLinux::GetSensorDevice(
    mojom::SensorType type) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  auto sensor = sensor_devices_by_type_.find(type);
  if (sensor == sensor_devices_by_type_.end())
    return nullptr;
  return sensor->second.get();
}

void PlatformSensorProviderLinux::GetAllSensorDevices() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  // TODO(maksims): implement this method once we have discovery API.
  NOTIMPLEMENTED();
}

void PlatformSensorProviderLinux::SetSensorDeviceManagerForTesting(
    std::unique_ptr<SensorDeviceManager> sensor_device_manager) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  Shutdown();
  sensor_device_manager_ = std::move(sensor_device_manager);
}

void PlatformSensorProviderLinux::SetFileTaskRunnerForTesting(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  file_task_runner_ = std::move(task_runner);
}

void PlatformSensorProviderLinux::ProcessStoredRequests() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  std::vector<mojom::SensorType> request_types = GetPendingRequestTypes();
  if (request_types.empty())
    return;

  for (auto const& type : request_types) {
    if (IsFusionSensorType(type)) {
      SensorReadingSharedBuffer* reading_buffer =
          GetSensorReadingSharedBufferForType(type);
      CreateFusionSensor(
          type, reading_buffer,
          base::Bind(&PlatformSensorProviderLinux::NotifySensorCreated,
                     base::Unretained(this), type));
      continue;
    }

    SensorInfoLinux* device = nullptr;
    auto device_entry = sensor_devices_by_type_.find(type);
    if (device_entry != sensor_devices_by_type_.end())
      device = device_entry->second.get();
    CreateSensorAndNotify(type, device);
  }
}

void PlatformSensorProviderLinux::CreateSensorAndNotify(
    mojom::SensorType type,
    SensorInfoLinux* sensor_device) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  scoped_refptr<PlatformSensorLinux> sensor;
  SensorReadingSharedBuffer* reading_buffer =
      GetSensorReadingSharedBufferForType(type);
  if (sensor_device && reading_buffer && StartPollingThread()) {
    sensor = new PlatformSensorLinux(type, reading_buffer, this, sensor_device,
                                     polling_thread_->task_runner());
  }
  NotifySensorCreated(type, sensor);
}

void PlatformSensorProviderLinux::OnSensorNodesEnumerated() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(!sensor_nodes_enumerated_);
  sensor_nodes_enumerated_ = true;
  ProcessStoredRequests();
}

void PlatformSensorProviderLinux::OnDeviceAdded(
    mojom::SensorType type,
    std::unique_ptr<SensorInfoLinux> sensor_device) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  // At the moment, we support only one device per type.
  if (base::ContainsKey(sensor_devices_by_type_, type)) {
    DVLOG(1) << "Sensor ignored. Type " << type
             << ". Node: " << sensor_device->device_node;
    return;
  }
  sensor_devices_by_type_[type] = std::move(sensor_device);
}

void PlatformSensorProviderLinux::OnDeviceRemoved(
    mojom::SensorType type,
    const std::string& device_node) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  auto it = sensor_devices_by_type_.find(type);
  if (it != sensor_devices_by_type_.end() &&
      it->second->device_node == device_node) {
    sensor_devices_by_type_.erase(it);
  }
}

void PlatformSensorProviderLinux::CreateFusionSensor(
    mojom::SensorType type,
    SensorReadingSharedBuffer* reading_buffer,
    const CreateSensorCallback& callback) {
  DCHECK(IsFusionSensorType(type));
  std::unique_ptr<PlatformSensorFusionAlgorithm> fusion_algorithm;
  switch (type) {
    case mojom::SensorType::LINEAR_ACCELERATION:
      fusion_algorithm = std::make_unique<
          LinearAccelerationFusionAlgorithmUsingAccelerometer>();
      break;
    case mojom::SensorType::ABSOLUTE_ORIENTATION_EULER_ANGLES:
      fusion_algorithm = std::make_unique<
          AbsoluteOrientationEulerAnglesFusionAlgorithmUsingAccelerometerAndMagnetometer>();
      break;
    case mojom::SensorType::ABSOLUTE_ORIENTATION_QUATERNION:
      fusion_algorithm = std::make_unique<
          OrientationQuaternionFusionAlgorithmUsingEulerAngles>(
          true /* absolute */);
      break;
    case mojom::SensorType::RELATIVE_ORIENTATION_EULER_ANGLES:
      if (GetSensorDevice(mojom::SensorType::GYROSCOPE)) {
        fusion_algorithm = std::make_unique<
            RelativeOrientationEulerAnglesFusionAlgorithmUsingAccelerometerAndGyroscope>();
      } else {
        fusion_algorithm = std::make_unique<
            RelativeOrientationEulerAnglesFusionAlgorithmUsingAccelerometer>();
      }
      break;
    case mojom::SensorType::RELATIVE_ORIENTATION_QUATERNION:
      fusion_algorithm = std::make_unique<
          OrientationQuaternionFusionAlgorithmUsingEulerAngles>(
          false /* absolute */);
      break;
    default:
      NOTREACHED();
  }

  DCHECK(fusion_algorithm);
  PlatformSensorFusion::Create(reading_buffer, this,
                               std::move(fusion_algorithm), callback);
}

}  // namespace device
