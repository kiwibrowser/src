// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/device_sensors/device_motion_event_pump.h"

#include <cmath>

#include "base/logging.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_thread.h"
#include "content/renderer/render_thread_impl.h"
#include "services/device/public/cpp/generic_sensor/motion_data.h"
#include "services/device/public/mojom/sensor.mojom.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "ui/gfx/geometry/angle_conversions.h"

namespace content {

template class DeviceSensorEventPump<blink::WebDeviceMotionListener>;

DeviceMotionEventPump::DeviceMotionEventPump(RenderThread* thread)
    : DeviceSensorEventPump<blink::WebDeviceMotionListener>(thread),
      accelerometer_(this, device::mojom::SensorType::ACCELEROMETER),
      linear_acceleration_sensor_(
          this,
          device::mojom::SensorType::LINEAR_ACCELERATION),
      gyroscope_(this, device::mojom::SensorType::GYROSCOPE) {}

DeviceMotionEventPump::~DeviceMotionEventPump() {}

void DeviceMotionEventPump::SendStartMessage() {
  // When running layout tests, those observers should not listen to the
  // actual hardware changes. In order to make that happen, don't connect
  // the other end of the mojo pipe to anything.
  //
  // TODO(sammc): Remove this when JS layout test support for shared buffers
  // is ready and the layout tests are converted to use that for mocking.
  // https://crbug.com/774183
  if (!RenderThreadImpl::current() ||
      RenderThreadImpl::current()->layout_test_mode()) {
    return;
  }

  SendStartMessageImpl();
}

void DeviceMotionEventPump::SendStopMessage() {
  // SendStopMessage() gets called both when the page visibility changes and if
  // all device motion event listeners are unregistered. Since removing the
  // event listener is more rare than the page visibility changing,
  // Sensor::Suspend() is used to optimize this case for not doing extra work.

  accelerometer_.Stop();
  linear_acceleration_sensor_.Stop();
  gyroscope_.Stop();
}

void DeviceMotionEventPump::SendFakeDataForTesting(void* fake_data) {
  if (!listener())
    return;

  device::MotionData data = *static_cast<device::MotionData*>(fake_data);
  listener()->DidChangeDeviceMotion(data);
}

void DeviceMotionEventPump::FireEvent() {
  device::MotionData data;
  // The device orientation spec states that interval should be in milliseconds.
  // https://w3c.github.io/deviceorientation/spec-source-orientation.html#devicemotion
  data.interval = kDefaultPumpDelayMicroseconds / 1000;

  DCHECK(listener());

  GetDataFromSharedMemory(&data);

  if (ShouldFireEvent(data))
    listener()->DidChangeDeviceMotion(data);
}

void DeviceMotionEventPump::SendStartMessageImpl() {
  if (!sensor_provider_) {
    RenderFrame* const render_frame = GetRenderFrame();
    if (!render_frame)
      return;

    render_frame->GetRemoteInterfaces()->GetInterface(
        mojo::MakeRequest(&sensor_provider_));
    sensor_provider_.set_connection_error_handler(
        base::BindOnce(&DeviceSensorEventPump::HandleSensorProviderError,
                       base::Unretained(this)));
  }

  accelerometer_.Start(sensor_provider_.get());
  linear_acceleration_sensor_.Start(sensor_provider_.get());
  gyroscope_.Start(sensor_provider_.get());
}

bool DeviceMotionEventPump::SensorsReadyOrErrored() const {
  return accelerometer_.ReadyOrErrored() &&
         linear_acceleration_sensor_.ReadyOrErrored() &&
         gyroscope_.ReadyOrErrored();
}

void DeviceMotionEventPump::GetDataFromSharedMemory(device::MotionData* data) {
  // "Active" here means that sensor has been initialized and is either ready
  // or not available.
  bool accelerometer_active = true;
  bool linear_acceleration_sensor_active = true;
  bool gyroscope_active = true;

  if (accelerometer_.SensorReadingCouldBeRead() &&
      (accelerometer_active = accelerometer_.reading.timestamp() != 0.0)) {
    data->acceleration_including_gravity_x = accelerometer_.reading.accel.x;
    data->acceleration_including_gravity_y = accelerometer_.reading.accel.y;
    data->acceleration_including_gravity_z = accelerometer_.reading.accel.z;
    data->has_acceleration_including_gravity_x =
        !std::isnan(accelerometer_.reading.accel.x.value());
    data->has_acceleration_including_gravity_y =
        !std::isnan(accelerometer_.reading.accel.y.value());
    data->has_acceleration_including_gravity_z =
        !std::isnan(accelerometer_.reading.accel.z.value());
  }

  if (linear_acceleration_sensor_.SensorReadingCouldBeRead() &&
      (linear_acceleration_sensor_active =
           linear_acceleration_sensor_.reading.timestamp() != 0.0)) {
    data->acceleration_x = linear_acceleration_sensor_.reading.accel.x;
    data->acceleration_y = linear_acceleration_sensor_.reading.accel.y;
    data->acceleration_z = linear_acceleration_sensor_.reading.accel.z;
    data->has_acceleration_x =
        !std::isnan(linear_acceleration_sensor_.reading.accel.x.value());
    data->has_acceleration_y =
        !std::isnan(linear_acceleration_sensor_.reading.accel.y.value());
    data->has_acceleration_z =
        !std::isnan(linear_acceleration_sensor_.reading.accel.z.value());
  }

  if (gyroscope_.SensorReadingCouldBeRead() &&
      (gyroscope_active = gyroscope_.reading.timestamp() != 0.0)) {
    data->rotation_rate_alpha = gfx::RadToDeg(gyroscope_.reading.gyro.x);
    data->rotation_rate_beta = gfx::RadToDeg(gyroscope_.reading.gyro.y);
    data->rotation_rate_gamma = gfx::RadToDeg(gyroscope_.reading.gyro.z);
    data->has_rotation_rate_alpha =
        !std::isnan(gyroscope_.reading.gyro.x.value());
    data->has_rotation_rate_beta =
        !std::isnan(gyroscope_.reading.gyro.y.value());
    data->has_rotation_rate_gamma =
        !std::isnan(gyroscope_.reading.gyro.z.value());
  }

  data->all_available_sensors_are_active = accelerometer_active &&
                                           linear_acceleration_sensor_active &&
                                           gyroscope_active;
}  // namespace content

bool DeviceMotionEventPump::ShouldFireEvent(
    const device::MotionData& data) const {
  return data.all_available_sensors_are_active;
}

}  // namespace content
