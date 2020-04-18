// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <math.h>

#include "base/memory/ptr_util.h"
#include "base/numerics/math_constants.h"
#include "base/time/time.h"
#include "device/vr/orientation/orientation_device.h"
#include "services/device/public/cpp/generic_sensor/sensor_reading_shared_buffer_reader.h"
#include "services/device/public/mojom/sensor_provider.mojom.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/gfx/geometry/quaternion.h"
#include "ui/gfx/geometry/vector3d_f.h"

namespace device {

using gfx::Quaternion;
using gfx::Vector3dF;

namespace {
static constexpr int kDefaultPumpFrequencyHz = 60;

mojom::VRDisplayInfoPtr CreateVRDisplayInfo(unsigned int id) {
  static const char DEVICE_NAME[] = "VR Orientation Device";

  mojom::VRDisplayInfoPtr display_info = mojom::VRDisplayInfo::New();
  display_info->index = id;
  display_info->displayName = DEVICE_NAME;
  display_info->capabilities = mojom::VRDisplayCapabilities::New();
  display_info->capabilities->hasPosition = false;
  display_info->capabilities->hasExternalDisplay = false;
  display_info->capabilities->canPresent = false;

  return display_info;
}

display::Display::Rotation GetRotation() {
  display::Screen* screen = display::Screen::GetScreen();
  if (!screen) {
    // If we can't get rotation we'll assume it's 0.
    return display::Display::ROTATE_0;
  }

  return screen->GetPrimaryDisplay().rotation();
}

}  // namespace

VROrientationDevice::VROrientationDevice(
    mojom::SensorProviderPtr* sensor_provider,
    base::OnceClosure ready_callback)
    : ready_callback_(std::move(ready_callback)), binding_(this) {
  (*sensor_provider)
      ->GetSensor(kOrientationSensorType,
                  base::BindOnce(&VROrientationDevice::SensorReady,
                                 base::Unretained(this)));

  SetVRDisplayInfo(CreateVRDisplayInfo(GetId()));
}

VROrientationDevice::~VROrientationDevice() = default;

void VROrientationDevice::SensorReady(
    device::mojom::SensorCreationResult,
    device::mojom::SensorInitParamsPtr params) {
  if (!params) {
    // This means that there are no orientation sensors on this device.
    HandleSensorError();
    std::move(ready_callback_).Run();
    return;
  }

  constexpr size_t kReadBufferSize = sizeof(device::SensorReadingSharedBuffer);

  DCHECK_EQ(0u, params->buffer_offset % kReadBufferSize);

  device::PlatformSensorConfiguration default_config =
      params->default_configuration;

  sensor_.Bind(std::move(params->sensor));

  binding_.Bind(std::move(params->client_request));

  shared_buffer_handle_ = std::move(params->memory);
  DCHECK(!shared_buffer_);
  shared_buffer_ = shared_buffer_handle_->MapAtOffset(kReadBufferSize,
                                                      params->buffer_offset);

  if (!shared_buffer_) {
    // If we cannot read data, we cannot supply a device.
    HandleSensorError();
    std::move(ready_callback_).Run();
    return;
  }

  const device::SensorReadingSharedBuffer* buffer =
      static_cast<const device::SensorReadingSharedBuffer*>(
          shared_buffer_.get());
  shared_buffer_reader_.reset(
      new device::SensorReadingSharedBufferReader(buffer));

  default_config.set_frequency(kDefaultPumpFrequencyHz);
  sensor_.set_connection_error_handler(base::BindOnce(
      &VROrientationDevice::HandleSensorError, base::Unretained(this)));
  sensor_->ConfigureReadingChangeNotifications(false /* disabled */);
  sensor_->AddConfiguration(
      default_config,
      base::BindOnce(&VROrientationDevice::OnSensorAddConfiguration,
                     base::Unretained(this)));
}

// Mojo callback for Sensor::AddConfiguration().
void VROrientationDevice::OnSensorAddConfiguration(bool success) {
  if (!success) {
    // Sensor config is not supported so we can't provide sensor events.
    HandleSensorError();
  } else {
    // We're good to go.
    available_ = true;
  }

  std::move(ready_callback_).Run();
}

void VROrientationDevice::RaiseError() {
  HandleSensorError();
}

void VROrientationDevice::HandleSensorError() {
  sensor_.reset();
  shared_buffer_handle_.reset();
  shared_buffer_.reset();
  binding_.Close();
}

void VROrientationDevice::OnMagicWindowPoseRequest(
    mojom::VRMagicWindowProvider::GetPoseCallback callback) {
  mojom::VRPosePtr pose = mojom::VRPose::New();
  pose->orientation.emplace(4);

  SensorReading latest_reading;
  // If the reading fails just return the last pose that we got.
  if (shared_buffer_reader_->GetReading(&latest_reading)) {
    latest_pose_.set_x(latest_reading.orientation_quat.x);
    latest_pose_.set_y(latest_reading.orientation_quat.y);
    latest_pose_.set_z(latest_reading.orientation_quat.z);
    latest_pose_.set_w(latest_reading.orientation_quat.w);

    latest_pose_ =
        WorldSpaceToUserOrientedSpace(SensorSpaceToWorldSpace(latest_pose_));
  }

  pose->orientation.value()[0] = latest_pose_.x();
  pose->orientation.value()[1] = latest_pose_.y();
  pose->orientation.value()[2] = latest_pose_.z();
  pose->orientation.value()[3] = latest_pose_.w();

  std::move(callback).Run(std::move(pose));
}

Quaternion VROrientationDevice::SensorSpaceToWorldSpace(Quaternion q) {
  display::Display::Rotation rotation = GetRotation();

  if (rotation == display::Display::ROTATE_90) {
    // Rotate the sensor reading to account for the screen rotation.
    q = q * Quaternion(Vector3dF(0, 0, 1), -base::kPiDouble / 2);
  } else if (rotation == display::Display::ROTATE_270) {
    // Rotate the sensor reading to account for the screen rotation the other
    // way.
    q = q * Quaternion(Vector3dF(0, 0, 1), base::kPiDouble / 2);
  }

  // Tilt the view up to have the y axis as the vertical axis instead of z
  q = Quaternion(Vector3dF(1, 0, 0), -base::kPiDouble / 2) * q;

  return q;
}

Quaternion VROrientationDevice::WorldSpaceToUserOrientedSpace(Quaternion q) {
  if (!base_pose_) {
    // Check that q is valid by checking if the length is not 0 (it should
    // technically always be 1, but this accounts for rounding errors).
    if (!(q.Length() > .1)) {
      // q is invalid. Do not use for base pose.
      return q;
    }

    // A base pose to read the initial forward direction off of.
    base_pose_ = q;

    // Extract the yaw from base pose to use as the base forward direction.
    base_pose_->set_x(0);
    base_pose_->set_z(0);
    base_pose_ = base_pose_->Normalized();
  }

  // Adjust the base forward on the orientation to where the original forward
  // was.
  q = base_pose_->inverse() * q;

  return q;
}

bool VROrientationDevice::IsFallbackDevice() {
  return true;
};

}  // namespace device
