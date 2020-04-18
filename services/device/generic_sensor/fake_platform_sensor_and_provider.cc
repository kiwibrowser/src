// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/device/generic_sensor/fake_platform_sensor_and_provider.h"

#include <utility>

using ::testing::Invoke;
using ::testing::Return;
using ::testing::_;

namespace device {

FakePlatformSensor::FakePlatformSensor(
    mojom::SensorType type,
    SensorReadingSharedBuffer* reading_buffer,
    PlatformSensorProvider* provider)
    : PlatformSensor(type, reading_buffer, provider) {
  ON_CALL(*this, StartSensor(_))
      .WillByDefault(
          Invoke([this](const PlatformSensorConfiguration& configuration) {
            SensorReading reading;
            // Only mocking the shared memory update for AMBIENT_LIGHT type is
            // enough.
            if (GetType() == mojom::SensorType::AMBIENT_LIGHT) {
              // Set the shared buffer value as frequency for testing purpose.
              reading.als.value = configuration.frequency();
              UpdateSharedBufferAndNotifyClients(reading);
            }
            return true;
          }));
}

FakePlatformSensor::~FakePlatformSensor() = default;

bool FakePlatformSensor::CheckSensorConfiguration(
    const PlatformSensorConfiguration& configuration) {
  return configuration.frequency() <= GetMaximumSupportedFrequency() &&
         configuration.frequency() >= GetMinimumSupportedFrequency();
}

PlatformSensorConfiguration FakePlatformSensor::GetDefaultConfiguration() {
  return PlatformSensorConfiguration(30.0);
}

mojom::ReportingMode FakePlatformSensor::GetReportingMode() {
  // Set the ReportingMode as ON_CHANGE, so we can test the
  // SensorReadingChanged() mojo interface.
  return mojom::ReportingMode::ON_CHANGE;
}

double FakePlatformSensor::GetMaximumSupportedFrequency() {
  return maximum_supported_frequency_;
}

double FakePlatformSensor::GetMinimumSupportedFrequency() {
  return 1.0;
}

FakePlatformSensorProvider::FakePlatformSensorProvider() {
  ON_CALL(*this, DoCreateSensorInternal(_, _, _))
      .WillByDefault(Invoke(
          [](mojom::SensorType, scoped_refptr<PlatformSensor> sensor,
             const PlatformSensorProvider::CreateSensorCallback& callback) {
            callback.Run(std::move(sensor));
          }));
}

FakePlatformSensorProvider::~FakePlatformSensorProvider() = default;

SensorReadingSharedBuffer* FakePlatformSensorProvider::GetSensorReadingBuffer(
    mojom::SensorType type) {
  return CreateSharedBufferIfNeeded()
             ? GetSensorReadingSharedBufferForType(type)
             : nullptr;
}

void FakePlatformSensorProvider::CreateSensorInternal(
    mojom::SensorType type,
    SensorReadingSharedBuffer* reading_buffer,
    const CreateSensorCallback& callback) {
  DCHECK(type >= mojom::SensorType::FIRST && type <= mojom::SensorType::LAST);
  auto sensor =
      base::MakeRefCounted<FakePlatformSensor>(type, reading_buffer, this);
  DoCreateSensorInternal(type, std::move(sensor), callback);
}

MockPlatformSensorClient::MockPlatformSensorClient() {
  ON_CALL(*this, IsSuspended()).WillByDefault(Return(false));
}

MockPlatformSensorClient::MockPlatformSensorClient(
    scoped_refptr<PlatformSensor> sensor)
    : MockPlatformSensorClient() {
  DCHECK(sensor);
  sensor_ = std::move(sensor);
  sensor_->AddClient(this);
}

MockPlatformSensorClient::~MockPlatformSensorClient() {
  if (sensor_)
    sensor_->RemoveClient(this);
}

}  // namespace device
