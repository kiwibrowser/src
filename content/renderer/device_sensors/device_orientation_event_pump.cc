// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/device_sensors/device_orientation_event_pump.h"

#include <cmath>

#include "base/logging.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_thread.h"
#include "content/renderer/render_thread_impl.h"
#include "services/device/public/mojom/sensor.mojom.h"
#include "services/service_manager/public/cpp/interface_provider.h"

namespace {

bool IsAngleDifferentThreshold(bool has_angle1,
                               double angle1,
                               bool has_angle2,
                               double angle2) {
  if (has_angle1 != has_angle2)
    return true;

  return (has_angle1 &&
          std::fabs(angle1 - angle2) >=
              content::DeviceOrientationEventPump::kOrientationThreshold);
}

bool IsSignificantlyDifferent(const device::OrientationData& data1,
                              const device::OrientationData& data2) {
  return IsAngleDifferentThreshold(data1.has_alpha, data1.alpha,
                                   data2.has_alpha, data2.alpha) ||
         IsAngleDifferentThreshold(data1.has_beta, data1.beta, data2.has_beta,
                                   data2.beta) ||
         IsAngleDifferentThreshold(data1.has_gamma, data1.gamma,
                                   data2.has_gamma, data2.gamma);
}

}  // namespace

namespace content {

template class DeviceSensorEventPump<blink::WebDeviceOrientationListener>;

const double DeviceOrientationEventPump::kOrientationThreshold = 0.1;

DeviceOrientationEventPump::DeviceOrientationEventPump(RenderThread* thread,
                                                       bool absolute)
    : DeviceSensorEventPump<blink::WebDeviceOrientationListener>(thread),
      relative_orientation_sensor_(
          this,
          device::mojom::SensorType::RELATIVE_ORIENTATION_EULER_ANGLES),
      absolute_orientation_sensor_(
          this,
          device::mojom::SensorType::ABSOLUTE_ORIENTATION_EULER_ANGLES),
      absolute_(absolute),
      fall_back_to_absolute_orientation_sensor_(!absolute) {}

DeviceOrientationEventPump::~DeviceOrientationEventPump() {}

void DeviceOrientationEventPump::SendStartMessage() {
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

void DeviceOrientationEventPump::SendStopMessage() {
  // SendStopMessage() gets called both when the page visibility changes and if
  // all device orientation event listeners are unregistered. Since removing
  // the event listener is more rare than the page visibility changing,
  // Sensor::Suspend() is used to optimize this case for not doing extra work.

  relative_orientation_sensor_.Stop();
  // This is needed in case we fallback to using the absolute orientation
  // sensor. In this case, the relative orientation sensor is marked as
  // SensorState::SHOULD_SUSPEND, and if the relative orientation sensor
  // is not available, the absolute orientation sensor should also be marked as
  // SensorState::SHOULD_SUSPEND, but only after the
  // absolute_orientation_sensor_.Start() is called for initializing
  // the absolute orientation sensor in
  // DeviceOrientationEventPump::DidStartIfPossible().
  if (relative_orientation_sensor_.sensor_state ==
          SensorState::SHOULD_SUSPEND &&
      fall_back_to_absolute_orientation_sensor_) {
    should_suspend_absolute_orientation_sensor_ = true;
  }

  absolute_orientation_sensor_.Stop();
}

void DeviceOrientationEventPump::SendFakeDataForTesting(void* fake_data) {
  if (!listener())
    return;

  device::OrientationData data =
      *static_cast<device::OrientationData*>(fake_data);
  listener()->DidChangeDeviceOrientation(data);
}

void DeviceOrientationEventPump::FireEvent() {
  device::OrientationData data;

  DCHECK(listener());

  GetDataFromSharedMemory(&data);

  if (ShouldFireEvent(data)) {
    data_ = data;
    listener()->DidChangeDeviceOrientation(data);
  }
}

void DeviceOrientationEventPump::DidStartIfPossible() {
  if (!absolute_ && !relative_orientation_sensor_.sensor &&
      fall_back_to_absolute_orientation_sensor_ && sensor_provider_) {
    // When relative orientation sensor is not available fall back to using
    // the absolute orientation sensor but only on the first failure.
    fall_back_to_absolute_orientation_sensor_ = false;
    absolute_orientation_sensor_.Start(sensor_provider_.get());
    if (should_suspend_absolute_orientation_sensor_) {
      // The absolute orientation sensor needs to be marked as
      // SensorState::SUSPENDED when it is successfully initialized.
      absolute_orientation_sensor_.sensor_state = SensorState::SHOULD_SUSPEND;
      should_suspend_absolute_orientation_sensor_ = false;
    }
    return;
  }
  DeviceSensorEventPump::DidStartIfPossible();
}

void DeviceOrientationEventPump::SendStartMessageImpl() {
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

  if (absolute_) {
    absolute_orientation_sensor_.Start(sensor_provider_.get());
  } else {
    fall_back_to_absolute_orientation_sensor_ = true;
    should_suspend_absolute_orientation_sensor_ = false;
    relative_orientation_sensor_.Start(sensor_provider_.get());
  }
}

bool DeviceOrientationEventPump::SensorsReadyOrErrored() const {
  if (!relative_orientation_sensor_.ReadyOrErrored() ||
      !absolute_orientation_sensor_.ReadyOrErrored()) {
    return false;
  }

  // At most one sensor can be successfully initialized.
  DCHECK(!relative_orientation_sensor_.sensor ||
         !absolute_orientation_sensor_.sensor);

  return true;
}

void DeviceOrientationEventPump::GetDataFromSharedMemory(
    device::OrientationData* data) {
  data->all_available_sensors_are_active = true;

  if (!absolute_ && relative_orientation_sensor_.SensorReadingCouldBeRead()) {
    // For DeviceOrientation Event, this provides relative orientation data.
    data->all_available_sensors_are_active =
        relative_orientation_sensor_.reading.timestamp() != 0.0;
    if (!data->all_available_sensors_are_active)
      return;
    data->alpha = relative_orientation_sensor_.reading.orientation_euler.z;
    data->beta = relative_orientation_sensor_.reading.orientation_euler.x;
    data->gamma = relative_orientation_sensor_.reading.orientation_euler.y;
    data->has_alpha = !std::isnan(
        relative_orientation_sensor_.reading.orientation_euler.z.value());
    data->has_beta = !std::isnan(
        relative_orientation_sensor_.reading.orientation_euler.x.value());
    data->has_gamma = !std::isnan(
        relative_orientation_sensor_.reading.orientation_euler.y.value());
    data->absolute = false;
  } else if (absolute_orientation_sensor_.SensorReadingCouldBeRead()) {
    // For DeviceOrientationAbsolute Event, this provides absolute orientation
    // data.
    //
    // For DeviceOrientation Event, this provides absolute orientation data if
    // relative orientation data is not available.
    data->all_available_sensors_are_active =
        absolute_orientation_sensor_.reading.timestamp() != 0.0;
    if (!data->all_available_sensors_are_active)
      return;
    data->alpha = absolute_orientation_sensor_.reading.orientation_euler.z;
    data->beta = absolute_orientation_sensor_.reading.orientation_euler.x;
    data->gamma = absolute_orientation_sensor_.reading.orientation_euler.y;
    data->has_alpha = !std::isnan(
        absolute_orientation_sensor_.reading.orientation_euler.z.value());
    data->has_beta = !std::isnan(
        absolute_orientation_sensor_.reading.orientation_euler.x.value());
    data->has_gamma = !std::isnan(
        absolute_orientation_sensor_.reading.orientation_euler.y.value());
    data->absolute = true;
  } else {
    data->absolute = absolute_;
  }
}

bool DeviceOrientationEventPump::ShouldFireEvent(
    const device::OrientationData& data) const {
  if (!data.all_available_sensors_are_active)
    return false;

  if (!data.has_alpha && !data.has_beta && !data.has_gamma) {
    // no data can be provided, this is an all-null event.
    return true;
  }

  return IsSignificantlyDifferent(data_, data);
}

}  // namespace content
