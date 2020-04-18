// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/device/generic_sensor/platform_sensor_provider_win.h"

#include <comdef.h>
#include <objbase.h>

#include <iomanip>

#include "base/memory/singleton.h"
#include "base/task_runner_util.h"
#include "base/threading/thread.h"
#include "services/device/generic_sensor/linear_acceleration_fusion_algorithm_using_accelerometer.h"
#include "services/device/generic_sensor/platform_sensor_fusion.h"
#include "services/device/generic_sensor/platform_sensor_win.h"

namespace device {

class PlatformSensorProviderWin::SensorThread final : public base::Thread {
 public:
  SensorThread() : base::Thread("Sensor thread") { init_com_with_mta(true); }

  void SetSensorManagerForTesting(
      Microsoft::WRL::ComPtr<ISensorManager> sensor_manager) {
    sensor_manager_ = sensor_manager;
  }

  const Microsoft::WRL::ComPtr<ISensorManager>& sensor_manager() const {
    return sensor_manager_;
  }

 protected:
  void Init() override {
    if (sensor_manager_)
      return;
    HRESULT hr = ::CoCreateInstance(CLSID_SensorManager, nullptr, CLSCTX_ALL,
                                    IID_PPV_ARGS(&sensor_manager_));
    if (FAILED(hr)) {
      // Only log this error the first time.
      static bool logged_failure = false;
      if (!logged_failure) {
        LOG(ERROR) << "Unable to create instance of SensorManager: "
                   << _com_error(hr).ErrorMessage() << " (0x" << std::hex
                   << std::uppercase << std::setfill('0') << std::setw(8) << hr
                   << ")";
        logged_failure = true;
      }
    }
  }

  void CleanUp() override { sensor_manager_.Reset(); }

 private:
  Microsoft::WRL::ComPtr<ISensorManager> sensor_manager_;
};

// static
PlatformSensorProviderWin* PlatformSensorProviderWin::GetInstance() {
  return base::Singleton<
      PlatformSensorProviderWin,
      base::LeakySingletonTraits<PlatformSensorProviderWin>>::get();
}

void PlatformSensorProviderWin::SetSensorManagerForTesting(
    Microsoft::WRL::ComPtr<ISensorManager> sensor_manager) {
  CreateSensorThread();
  sensor_thread_->SetSensorManagerForTesting(sensor_manager);
}

PlatformSensorProviderWin::PlatformSensorProviderWin() = default;
PlatformSensorProviderWin::~PlatformSensorProviderWin() = default;

void PlatformSensorProviderWin::CreateSensorInternal(
    mojom::SensorType type,
    SensorReadingSharedBuffer* reading_buffer,
    const CreateSensorCallback& callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (!StartSensorThread()) {
    callback.Run(nullptr);
    return;
  }

  switch (type) {
    // Fusion sensor.
    case mojom::SensorType::LINEAR_ACCELERATION: {
      auto linear_acceleration_fusion_algorithm = std::make_unique<
          LinearAccelerationFusionAlgorithmUsingAccelerometer>();
      // If this PlatformSensorFusion object is successfully initialized,
      // |callback| will be run with a reference to this object.
      PlatformSensorFusion::Create(
          reading_buffer, this, std::move(linear_acceleration_fusion_algorithm),
          callback);
      break;
    }

    // Try to create low-level sensors by default.
    default: {
      base::PostTaskAndReplyWithResult(
          sensor_thread_->task_runner().get(), FROM_HERE,
          base::Bind(&PlatformSensorProviderWin::CreateSensorReader,
                     base::Unretained(this), type),
          base::Bind(&PlatformSensorProviderWin::SensorReaderCreated,
                     base::Unretained(this), type, reading_buffer, callback));
      break;
    }
  }
}

void PlatformSensorProviderWin::FreeResources() {
  StopSensorThread();
}

void PlatformSensorProviderWin::CreateSensorThread() {
  if (!sensor_thread_)
    sensor_thread_ = std::make_unique<SensorThread>();
}

bool PlatformSensorProviderWin::StartSensorThread() {
  CreateSensorThread();
  if (!sensor_thread_->IsRunning())
    return sensor_thread_->Start();
  return true;
}

void PlatformSensorProviderWin::StopSensorThread() {
  if (sensor_thread_ && sensor_thread_->IsRunning())
    sensor_thread_->Stop();
}

void PlatformSensorProviderWin::SensorReaderCreated(
    mojom::SensorType type,
    SensorReadingSharedBuffer* reading_buffer,
    const CreateSensorCallback& callback,
    std::unique_ptr<PlatformSensorReaderWin> sensor_reader) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (!sensor_reader) {
    callback.Run(nullptr);
    return;
  }

  scoped_refptr<PlatformSensor> sensor = new PlatformSensorWin(
      type, reading_buffer, this, sensor_thread_->task_runner(),
      std::move(sensor_reader));
  callback.Run(sensor);
}

std::unique_ptr<PlatformSensorReaderWin>
PlatformSensorProviderWin::CreateSensorReader(mojom::SensorType type) {
  DCHECK(sensor_thread_->task_runner()->BelongsToCurrentThread());
  if (!sensor_thread_->sensor_manager())
    return nullptr;
  return PlatformSensorReaderWin::Create(type,
                                         sensor_thread_->sensor_manager());
}

}  // namespace device
