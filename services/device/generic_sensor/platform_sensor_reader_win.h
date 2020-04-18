// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_DEVICE_GENERIC_SENSOR_PLATFORM_SENSOR_READER_WIN_H_
#define SERVICES_DEVICE_GENERIC_SENSOR_PLATFORM_SENSOR_READER_WIN_H_

#include <SensorsApi.h>
#include <wrl/client.h>

#include "services/device/public/mojom/sensor.mojom.h"

namespace device {

class PlatformSensorConfiguration;
struct ReaderInitParams;
union SensorReading;

// Generic class that uses ISensor interface to fetch sensor data. Used
// by PlatformSensorWin and delivers notifications via Client interface.
// Instances of this class must be created and destructed on the same thread.
class PlatformSensorReaderWin {
 public:
  // Client interface that can be used to receive notifications about sensor
  // error or data change events.
  class Client {
   public:
    virtual void OnReadingUpdated(const SensorReading& reading) = 0;
    virtual void OnSensorError() = 0;

   protected:
    virtual ~Client() {}
  };

  static std::unique_ptr<PlatformSensorReaderWin> Create(
      mojom::SensorType type,
      Microsoft::WRL::ComPtr<ISensorManager> sensor_manager);

  // Following methods are thread safe.
  void SetClient(Client* client);
  unsigned long GetMinimalReportingIntervalMs() const;
  bool StartSensor(const PlatformSensorConfiguration& configuration);
  void StopSensor();

  // Must be destructed on the same thread that was used during construction.
  ~PlatformSensorReaderWin();

 private:
  PlatformSensorReaderWin(Microsoft::WRL::ComPtr<ISensor> sensor,
                          std::unique_ptr<ReaderInitParams> params);

  static Microsoft::WRL::ComPtr<ISensor> GetSensorForType(
      REFSENSOR_TYPE_ID sensor_type,
      Microsoft::WRL::ComPtr<ISensorManager> sensor_manager);

  bool SetReportingInterval(const PlatformSensorConfiguration& configuration);
  void ListenSensorEvent();
  HRESULT SensorReadingChanged(ISensorDataReport* report,
                               SensorReading* reading) const;
  void SensorError();

 private:
  friend class EventListener;

  const std::unique_ptr<ReaderInitParams> init_params_;
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  // Following class members are protected by lock, because SetClient,
  // StartSensor and StopSensor are called from another thread by
  // PlatformSensorWin that can modify internal state of the object.
  base::Lock lock_;
  bool sensor_active_;
  Client* client_;
  Microsoft::WRL::ComPtr<ISensor> sensor_;
  scoped_refptr<EventListener> event_listener_;
  base::WeakPtrFactory<PlatformSensorReaderWin> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PlatformSensorReaderWin);
};

}  // namespace device

#endif  // SERVICES_DEVICE_GENERIC_SENSOR_PLATFORM_SENSOR_READER_WIN_H_
