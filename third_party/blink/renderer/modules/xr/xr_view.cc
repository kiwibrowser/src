// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/xr/xr_view.h"

#include "third_party/blink/renderer/modules/xr/xr_presentation_frame.h"
#include "third_party/blink/renderer/modules/xr/xr_session.h"
#include "third_party/blink/renderer/platform/geometry/float_point_3d.h"

namespace blink {

XRView::XRView(XRSession* session, Eye eye)
    : eye_(eye),
      session_(session),
      projection_matrix_(DOMFloat32Array::Create(16)) {
  eye_string_ = (eye_ == kEyeLeft ? "left" : "right");
}

XRSession* XRView::session() const {
  return session_;
}

// TODO(http://crbug.com/836496): This method only supports
// straight-ahead projection matrices. In order to support
// multiple sessions embedded with projection matrices that act
// like views into the shared camera space, this math needs to
// be updated.
void XRView::UpdateProjectionMatrixFromRawValues(
    const WTF::Vector<float>& projection_matrix,
    float near_depth,
    float far_depth) {
  DCHECK_EQ(projection_matrix.size(), 16lu);
  float* out = projection_matrix_->Data();
  for (int i = 0; i < 16; i++) {
    out[i] = projection_matrix[i];
  }

  // Recalculate elements that depend on near/far depth. The input matrix used
  // arbitrary values, need to adjust to what the client uses.
  float inverse_near_far = 1.0f / (near_depth - far_depth);
  out[10] = (near_depth + far_depth) * inverse_near_far;
  out[14] = (2.0f * far_depth * near_depth) * inverse_near_far;
}

void XRView::UpdateProjectionMatrixFromFoV(float up_rad,
                                           float down_rad,
                                           float left_rad,
                                           float right_rad,
                                           float near_depth,
                                           float far_depth) {
  float up_tan = tanf(up_rad);
  float down_tan = tanf(down_rad);
  float left_tan = tanf(left_rad);
  float right_tan = tanf(right_rad);
  float x_scale = 2.0f / (left_tan + right_tan);
  float y_scale = 2.0f / (up_tan + down_tan);
  float inv_nf = 1.0f / (near_depth - far_depth);

  float* out = projection_matrix_->Data();
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
  out[10] = (near_depth + far_depth) * inv_nf;
  out[11] = -1.0f;
  out[12] = 0.0f;
  out[13] = 0.0f;
  out[14] = (2.0f * far_depth * near_depth) * inv_nf;
  out[15] = 0.0f;

  inv_projection_dirty_ = true;
}

void XRView::UpdateProjectionMatrixFromAspect(float fovy,
                                              float aspect,
                                              float near_depth,
                                              float far_depth) {
  float f = 1.0f / tanf(fovy / 2);
  float inv_nf = 1.0f / (near_depth - far_depth);

  float* out = projection_matrix_->Data();
  out[0] = f / aspect;
  out[1] = 0.0f;
  out[2] = 0.0f;
  out[3] = 0.0f;
  out[4] = 0.0f;
  out[5] = f;
  out[6] = 0.0f;
  out[7] = 0.0f;
  out[8] = 0.0f;
  out[9] = 0.0f;
  out[10] = (far_depth + near_depth) * inv_nf;
  out[11] = -1.0f;
  out[12] = 0.0f;
  out[13] = 0.0f;
  out[14] = (2.0f * far_depth * near_depth) * inv_nf;
  out[15] = 0.0f;

  inv_projection_dirty_ = true;
}

void XRView::UpdateOffset(float x, float y, float z) {
  offset_.Set(x, y, z);
}

std::unique_ptr<TransformationMatrix> XRView::UnprojectPointer(
    double x,
    double y,
    double canvas_width,
    double canvas_height) {
  // Recompute the inverse projection matrix if needed.
  if (inv_projection_dirty_) {
    float* m = projection_matrix_->Data();
    std::unique_ptr<TransformationMatrix> projection =
        TransformationMatrix::Create(m[0], m[1], m[2], m[3], m[4], m[5], m[6],
                                     m[7], m[8], m[9], m[10], m[11], m[12],
                                     m[13], m[14], m[15]);
    inv_projection_ = TransformationMatrix::Create(projection->Inverse());
    inv_projection_dirty_ = false;
  }

  // Transform the x/y coordinate into WebGL normalized device coordinates.
  // Z coordinate of -1 means the point will be projected onto the projection
  // matrix near plane.
  FloatPoint3D point_in_projection_space(
      x / canvas_width * 2.0 - 1.0,
      (canvas_height - y) / canvas_height * 2.0 - 1.0, -1.0);

  FloatPoint3D point_in_view_space =
      inv_projection_->MapPoint(point_in_projection_space);

  const FloatPoint3D kOrigin(0.0, 0.0, 0.0);
  const FloatPoint3D kUp(0.0, 1.0, 0.0);

  // Generate a "Look At" matrix
  FloatPoint3D z_axis = kOrigin - point_in_view_space;
  z_axis.Normalize();

  FloatPoint3D x_axis = kUp.Cross(z_axis);
  x_axis.Normalize();

  FloatPoint3D y_axis = z_axis.Cross(x_axis);
  y_axis.Normalize();

  // TODO(bajones): There's probably a more efficent way to do this?
  TransformationMatrix inv_pointer(x_axis.X(), y_axis.X(), z_axis.X(), 0.0,
                                   x_axis.Y(), y_axis.Y(), z_axis.Y(), 0.0,
                                   x_axis.Z(), y_axis.Z(), z_axis.Z(), 0.0, 0.0,
                                   0.0, 0.0, 1.0);
  inv_pointer.Translate3d(-point_in_view_space.X(), -point_in_view_space.Y(),
                          -point_in_view_space.Z());

  // LookAt matrices are view matrices (inverted), so invert before returning.
  std::unique_ptr<TransformationMatrix> pointer =
      TransformationMatrix::Create(inv_pointer.Inverse());

  return pointer;
}

void XRView::Trace(blink::Visitor* visitor) {
  visitor->Trace(session_);
  visitor->Trace(projection_matrix_);
  ScriptWrappable::Trace(visitor);
}

}  // namespace blink
