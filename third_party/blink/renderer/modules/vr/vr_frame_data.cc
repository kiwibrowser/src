// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/vr/vr_frame_data.h"

#include "third_party/blink/renderer/modules/vr/vr_eye_parameters.h"
#include "third_party/blink/renderer/modules/vr/vr_pose.h"

#include <cmath>

namespace blink {

// TODO(bajones): All of the matrix math here is temporary. It will be removed
// once the VRService has been updated to allow the underlying VR APIs to
// provide the projection and view matrices directly.

// Build a projection matrix from a field of view and near/far planes.
void ProjectionFromFieldOfView(DOMFloat32Array* out_array,
                               VRFieldOfView* fov,
                               float depth_near,
                               float depth_far) {
  float up_tan = tanf(fov->UpDegrees() * M_PI / 180.0);
  float down_tan = tanf(fov->DownDegrees() * M_PI / 180.0);
  float left_tan = tanf(fov->LeftDegrees() * M_PI / 180.0);
  float right_tan = tanf(fov->RightDegrees() * M_PI / 180.0);
  float x_scale = 2.0f / (left_tan + right_tan);
  float y_scale = 2.0f / (up_tan + down_tan);

  float* out = out_array->Data();
  out[0] = x_scale;
  out[1] = 0.0f;
  out[2] = 0.0f;
  out[3] = 0.0f;
  out[4] = 0.0f;
  out[5] = y_scale;
  out[6] = 0.0f;
  out[7] = 0.0f;
  out[8] = -((left_tan - right_tan) * x_scale * 0.5);
  out[9] = ((up_tan - down_tan) * y_scale * 0.5);
  out[10] = (depth_near + depth_far) / (depth_near - depth_far);
  out[11] = -1.0f;
  out[12] = 0.0f;
  out[13] = 0.0f;
  out[14] = (2 * depth_far * depth_near) / (depth_near - depth_far);
  out[15] = 0.0f;
}

// Create a matrix from a rotation and translation.
void MatrixfromRotationTranslation(
    DOMFloat32Array* out_array,
    const base::Optional<WTF::Vector<float>>& rotation,
    const base::Optional<WTF::Vector<float>>& translation) {
  // Quaternion math
  float x = !rotation ? 0.0f : rotation.value()[0];
  float y = !rotation ? 0.0f : rotation.value()[1];
  float z = !rotation ? 0.0f : rotation.value()[2];
  float w = !rotation ? 1.0f : rotation.value()[3];
  float x2 = x + x;
  float y2 = y + y;
  float z2 = z + z;

  float xx = x * x2;
  float xy = x * y2;
  float xz = x * z2;
  float yy = y * y2;
  float yz = y * z2;
  float zz = z * z2;
  float wx = w * x2;
  float wy = w * y2;
  float wz = w * z2;

  float* out = out_array->Data();
  out[0] = 1 - (yy + zz);
  out[1] = xy + wz;
  out[2] = xz - wy;
  out[3] = 0;
  out[4] = xy - wz;
  out[5] = 1 - (xx + zz);
  out[6] = yz + wx;
  out[7] = 0;
  out[8] = xz + wy;
  out[9] = yz - wx;
  out[10] = 1 - (xx + yy);
  out[11] = 0;
  out[12] = !translation ? 0.0f : translation.value()[0];
  out[13] = !translation ? 0.0f : translation.value()[1];
  out[14] = !translation ? 0.0f : translation.value()[2];
  out[15] = 1;
}

// Translate a matrix
void MatrixTranslate(DOMFloat32Array* out_array,
                     const DOMFloat32Array* translation) {
  if (!translation)
    return;

  float x = translation->Data()[0];
  float y = translation->Data()[1];
  float z = translation->Data()[2];

  float* out = out_array->Data();
  out[12] = out[0] * x + out[4] * y + out[8] * z + out[12];
  out[13] = out[1] * x + out[5] * y + out[9] * z + out[13];
  out[14] = out[2] * x + out[6] * y + out[10] * z + out[14];
  out[15] = out[3] * x + out[7] * y + out[11] * z + out[15];
}

bool MatrixInvert(DOMFloat32Array* out_array) {
  float* out = out_array->Data();
  float a00 = out[0];
  float a01 = out[1];
  float a02 = out[2];
  float a03 = out[3];
  float a10 = out[4];
  float a11 = out[5];
  float a12 = out[6];
  float a13 = out[7];
  float a20 = out[8];
  float a21 = out[9];
  float a22 = out[10];
  float a23 = out[11];
  float a30 = out[12];
  float a31 = out[13];
  float a32 = out[14];
  float a33 = out[15];

  float b00 = a00 * a11 - a01 * a10;
  float b01 = a00 * a12 - a02 * a10;
  float b02 = a00 * a13 - a03 * a10;
  float b03 = a01 * a12 - a02 * a11;
  float b04 = a01 * a13 - a03 * a11;
  float b05 = a02 * a13 - a03 * a12;
  float b06 = a20 * a31 - a21 * a30;
  float b07 = a20 * a32 - a22 * a30;
  float b08 = a20 * a33 - a23 * a30;
  float b09 = a21 * a32 - a22 * a31;
  float b10 = a21 * a33 - a23 * a31;
  float b11 = a22 * a33 - a23 * a32;

  // Calculate the determinant
  float det =
      b00 * b11 - b01 * b10 + b02 * b09 + b03 * b08 - b04 * b07 + b05 * b06;

  if (!det)
    return false;

  det = 1.0 / det;

  out[0] = (a11 * b11 - a12 * b10 + a13 * b09) * det;
  out[1] = (a02 * b10 - a01 * b11 - a03 * b09) * det;
  out[2] = (a31 * b05 - a32 * b04 + a33 * b03) * det;
  out[3] = (a22 * b04 - a21 * b05 - a23 * b03) * det;
  out[4] = (a12 * b08 - a10 * b11 - a13 * b07) * det;
  out[5] = (a00 * b11 - a02 * b08 + a03 * b07) * det;
  out[6] = (a32 * b02 - a30 * b05 - a33 * b01) * det;
  out[7] = (a20 * b05 - a22 * b02 + a23 * b01) * det;
  out[8] = (a10 * b10 - a11 * b08 + a13 * b06) * det;
  out[9] = (a01 * b08 - a00 * b10 - a03 * b06) * det;
  out[10] = (a30 * b04 - a31 * b02 + a33 * b00) * det;
  out[11] = (a21 * b02 - a20 * b04 - a23 * b00) * det;
  out[12] = (a11 * b07 - a10 * b09 - a12 * b06) * det;
  out[13] = (a00 * b09 - a01 * b07 + a02 * b06) * det;
  out[14] = (a31 * b01 - a30 * b03 - a32 * b00) * det;
  out[15] = (a20 * b03 - a21 * b01 + a22 * b00) * det;

  return true;
};

VRFrameData::VRFrameData() {
  left_projection_matrix_ = DOMFloat32Array::Create(16);
  left_view_matrix_ = DOMFloat32Array::Create(16);
  right_projection_matrix_ = DOMFloat32Array::Create(16);
  right_view_matrix_ = DOMFloat32Array::Create(16);
  pose_ = VRPose::Create();
}

bool VRFrameData::Update(const device::mojom::blink::VRPosePtr& pose,
                         VREyeParameters* left_eye,
                         VREyeParameters* right_eye,
                         float depth_near,
                         float depth_far) {
  VRFieldOfView* fov_left;
  VRFieldOfView* fov_right;
  if (left_eye && right_eye) {
    fov_left = left_eye->FieldOfView();
    fov_right = right_eye->FieldOfView();
  } else {
    DCHECK(!left_eye && !right_eye);
    // TODO(offenwanger): Look into making the projection matrixes null instead
    // of hard coding values. May break some apps.
    fov_left = fov_right = new VRFieldOfView(45, 45, 45, 45);
  }

  // Build the projection matrices
  ProjectionFromFieldOfView(left_projection_matrix_, fov_left, depth_near,
                            depth_far);
  ProjectionFromFieldOfView(right_projection_matrix_, fov_right, depth_near,
                            depth_far);

  // Build the view matrices
  MatrixfromRotationTranslation(left_view_matrix_, pose->orientation,
                                pose->position);
  MatrixfromRotationTranslation(right_view_matrix_, pose->orientation,
                                pose->position);

  if (left_eye && right_eye) {
    MatrixTranslate(left_view_matrix_, left_eye->offset());
    MatrixTranslate(right_view_matrix_, right_eye->offset());
  }

  if (!MatrixInvert(left_view_matrix_) || !MatrixInvert(right_view_matrix_))
    return false;

  // Set the pose
  pose_->SetPose(pose);

  return true;
}

void VRFrameData::Trace(blink::Visitor* visitor) {
  visitor->Trace(left_projection_matrix_);
  visitor->Trace(left_view_matrix_);
  visitor->Trace(right_projection_matrix_);
  visitor->Trace(right_view_matrix_);
  visitor->Trace(pose_);
  ScriptWrappable::Trace(visitor);
}

}  // namespace blink
