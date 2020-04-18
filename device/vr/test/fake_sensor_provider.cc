// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/vr/test/fake_sensor_provider.h"

#include "services/device/public/mojom/sensor.mojom.h"
#include "services/device/public/mojom/sensor_provider.mojom.h"

namespace device {

FakeSensorProvider::FakeSensorProvider() : binding_(this) {}

FakeSensorProvider::FakeSensorProvider(mojom::SensorProviderRequest request)
    : binding_(this) {
  binding_.Bind(std::move(request));
}

FakeSensorProvider::~FakeSensorProvider() {
  if (callback_) {
    std::move(callback_).Run(mojom::SensorCreationResult::ERROR_NOT_AVAILABLE,
                             nullptr);
  }
}

void FakeSensorProvider::Bind(mojo::ScopedMessagePipeHandle handle) {
  binding_.Bind(mojom::SensorProviderRequest(std::move(handle)));
}

void FakeSensorProvider::GetSensor(mojom::SensorType type,
                                   GetSensorCallback callback) {
  callback_ = std::move(callback);
}

void FakeSensorProvider::CallCallback(mojom::SensorInitParamsPtr param) {
  std::move(callback_).Run(mojom::SensorCreationResult::SUCCESS,
                           std::move(param));
}

}  // namespace device
