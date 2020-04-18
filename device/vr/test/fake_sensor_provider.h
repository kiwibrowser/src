// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_VR_TEST_FAKE_SENSOR_PROVIDER_H_
#define DEVICE_VR_TEST_FAKE_SENSOR_PROVIDER_H_

#include "mojo/public/cpp/bindings/binding.h"
#include "services/device/public/mojom/sensor.mojom.h"
#include "services/device/public/mojom/sensor_provider.mojom.h"

namespace device {

class FakeSensorProvider : public mojom::SensorProvider {
 public:
  FakeSensorProvider();
  explicit FakeSensorProvider(mojom::SensorProviderRequest request);
  ~FakeSensorProvider() override;

  void Bind(mojo::ScopedMessagePipeHandle handle);
  void GetSensor(mojom::SensorType type, GetSensorCallback callback) override;
  void CallCallback(mojom::SensorInitParamsPtr param);

 private:
  mojo::Binding<mojom::SensorProvider> binding_;
  GetSensorCallback callback_;
};

}  // namespace device

#endif  // DEVICE_VR_TEST_FAKE_SENSOR_PROVIDER_H_
