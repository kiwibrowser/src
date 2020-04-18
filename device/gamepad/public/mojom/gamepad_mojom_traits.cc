// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/gamepad/public/mojom/gamepad_mojom_traits.h"

#include "base/containers/span.h"

namespace mojo {

// static
void StructTraits<
    device::mojom::GamepadQuaternionDataView,
    device::GamepadQuaternion>::SetToNull(device::GamepadQuaternion* out) {
  memset(out, 0, sizeof(device::GamepadQuaternion));
  out->not_null = false;
}

// static
bool StructTraits<device::mojom::GamepadQuaternionDataView,
                  device::GamepadQuaternion>::
    Read(device::mojom::GamepadQuaternionDataView data,
         device::GamepadQuaternion* out) {
  out->not_null = true;
  out->x = data.x();
  out->y = data.y();
  out->z = data.z();
  out->w = data.w();
  return true;
}

// static
void StructTraits<device::mojom::GamepadVectorDataView,
                  device::GamepadVector>::SetToNull(device::GamepadVector*
                                                        out) {
  memset(out, 0, sizeof(device::GamepadVector));
  out->not_null = false;
}

// static
bool StructTraits<device::mojom::GamepadVectorDataView, device::GamepadVector>::
    Read(device::mojom::GamepadVectorDataView data,
         device::GamepadVector* out) {
  out->not_null = true;
  out->x = data.x();
  out->y = data.y();
  out->z = data.z();
  return true;
}

// static
bool StructTraits<device::mojom::GamepadButtonDataView, device::GamepadButton>::
    Read(device::mojom::GamepadButtonDataView data,
         device::GamepadButton* out) {
  out->pressed = data.pressed();
  out->touched = data.touched();
  out->value = data.value();
  return true;
}

// static
device::mojom::GamepadHapticActuatorType
EnumTraits<device::mojom::GamepadHapticActuatorType,
           device::GamepadHapticActuatorType>::
    ToMojom(device::GamepadHapticActuatorType input) {
  switch (input) {
    case device::GamepadHapticActuatorType::kVibration:
      return device::mojom::GamepadHapticActuatorType::
          GamepadHapticActuatorTypeVibration;
    case device::GamepadHapticActuatorType::kDualRumble:
      return device::mojom::GamepadHapticActuatorType::
          GamepadHapticActuatorTypeDualRumble;
  }

  NOTREACHED();
  return device::mojom::GamepadHapticActuatorType::
      GamepadHapticActuatorTypeVibration;
}

// static
bool EnumTraits<device::mojom::GamepadHapticActuatorType,
                device::GamepadHapticActuatorType>::
    FromMojom(device::mojom::GamepadHapticActuatorType input,
              device::GamepadHapticActuatorType* output) {
  switch (input) {
    case device::mojom::GamepadHapticActuatorType::
        GamepadHapticActuatorTypeVibration:
      *output = device::GamepadHapticActuatorType::kVibration;
      return true;
    case device::mojom::GamepadHapticActuatorType::
        GamepadHapticActuatorTypeDualRumble:
      *output = device::GamepadHapticActuatorType::kDualRumble;
      return true;
  }

  NOTREACHED();
  return false;
}

// static
void StructTraits<device::mojom::GamepadHapticActuatorDataView,
                  device::GamepadHapticActuator>::
    SetToNull(device::GamepadHapticActuator* out) {
  memset(out, 0, sizeof(device::GamepadHapticActuator));
  out->not_null = false;
}

// static
bool StructTraits<device::mojom::GamepadHapticActuatorDataView,
                  device::GamepadHapticActuator>::
    Read(device::mojom::GamepadHapticActuatorDataView data,
         device::GamepadHapticActuator* out) {
  out->not_null = true;
  if (!data.ReadType(&out->type))
    return false;
  return true;
}

// static
void StructTraits<device::mojom::GamepadPoseDataView,
                  device::GamepadPose>::SetToNull(device::GamepadPose* out) {
  memset(out, 0, sizeof(device::GamepadPose));
  out->not_null = false;
}

// static
bool StructTraits<device::mojom::GamepadPoseDataView, device::GamepadPose>::
    Read(device::mojom::GamepadPoseDataView data, device::GamepadPose* out) {
  out->not_null = true;
  if (!data.ReadOrientation(&out->orientation)) {
    return false;
  }
  out->has_orientation = out->orientation.not_null;

  if (!data.ReadPosition(&out->position)) {
    return false;
  }
  out->has_position = out->position.not_null;

  if (!data.ReadAngularVelocity(&out->angular_velocity)) {
    return false;
  }
  if (!data.ReadLinearVelocity(&out->linear_velocity)) {
    return false;
  }
  if (!data.ReadAngularAcceleration(&out->angular_acceleration)) {
    return false;
  }
  if (!data.ReadLinearAcceleration(&out->linear_acceleration)) {
    return false;
  }
  return true;
}

// static
device::mojom::GamepadHand
EnumTraits<device::mojom::GamepadHand, device::GamepadHand>::ToMojom(
    device::GamepadHand input) {
  switch (input) {
    case device::GamepadHand::kNone:
      return device::mojom::GamepadHand::GamepadHandNone;
    case device::GamepadHand::kLeft:
      return device::mojom::GamepadHand::GamepadHandLeft;
    case device::GamepadHand::kRight:
      return device::mojom::GamepadHand::GamepadHandRight;
  }

  NOTREACHED();
  return device::mojom::GamepadHand::GamepadHandNone;
}

// static
bool EnumTraits<device::mojom::GamepadHand, device::GamepadHand>::FromMojom(
    device::mojom::GamepadHand input,
    device::GamepadHand* output) {
  switch (input) {
    case device::mojom::GamepadHand::GamepadHandNone:
      *output = device::GamepadHand::kNone;
      return true;
    case device::mojom::GamepadHand::GamepadHandLeft:
      *output = device::GamepadHand::kLeft;
      return true;
    case device::mojom::GamepadHand::GamepadHandRight:
      *output = device::GamepadHand::kRight;
      return true;
  }

  NOTREACHED();
  return false;
}

// static
base::span<const uint16_t>
StructTraits<device::mojom::GamepadDataView, device::Gamepad>::id(
    const device::Gamepad& r) {
  size_t id_length = 0;
  while (id_length < device::Gamepad::kIdLengthCap && r.id[id_length] != 0) {
    id_length++;
  }
  return base::make_span(reinterpret_cast<const uint16_t*>(r.id), id_length);
}

// static
base::span<const uint16_t>
StructTraits<device::mojom::GamepadDataView, device::Gamepad>::mapping(
    const device::Gamepad& r) {
  size_t mapping_length = 0;
  while (mapping_length < device::Gamepad::kMappingLengthCap &&
         r.mapping[mapping_length] != 0) {
    mapping_length++;
  }
  return base::make_span(reinterpret_cast<const uint16_t*>(r.mapping),
                         mapping_length);
}

// static
bool StructTraits<device::mojom::GamepadDataView, device::Gamepad>::Read(
    device::mojom::GamepadDataView data,
    device::Gamepad* out) {
  out->connected = data.connected();

  memset(out->id, 0, sizeof(out->id));
  base::span<uint16_t> id(reinterpret_cast<uint16_t*>(out->id),
                          device::Gamepad::kIdLengthCap);
  if (!data.ReadId(&id)) {
    return false;
  }

  out->timestamp = data.timestamp();

  base::span<double> axes(out->axes);
  if (!data.ReadAxes(&axes)) {
    return false;
  }
  // static_cast is safe when "data.ReadAxes(&axes)" above returns true.
  out->axes_length = static_cast<unsigned>(axes.size());

  base::span<device::GamepadButton> buttons(out->buttons);
  if (!data.ReadButtons(&buttons)) {
    return false;
  }
  // static_cast is safe when "data.ReadButtons(&buttons)" above returns true.
  out->buttons_length = static_cast<unsigned>(buttons.size());

  if (!data.ReadVibrationActuator(&out->vibration_actuator))
    return false;

  memset(out->mapping, 0, sizeof(out->mapping));
  base::span<uint16_t> mapping(reinterpret_cast<uint16_t*>(out->mapping),
                               device::Gamepad::kMappingLengthCap);
  if (!data.ReadMapping(&mapping)) {
    return false;
  }

  if (!data.ReadPose(&out->pose)) {
    return false;
  }

  device::GamepadHand hand;
  if (!data.ReadHand(&hand)) {
    return false;
  }
  out->hand = hand;

  out->display_id = data.display_id();

  return true;
}

}  // namespace mojo
