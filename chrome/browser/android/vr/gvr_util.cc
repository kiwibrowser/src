// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/vr/gvr_util.h"

#include <cmath>

#include "chrome/browser/vr/elements/ui_element.h"
#include "ui/gfx/geometry/point_f.h"
#include "ui/gfx/transform.h"

namespace vr {

namespace {

float Clamp(float value, float min, float max) {
  return std::max(min, std::min(value, max));
}

constexpr float kMargin = 1.f * M_PI / 180;

}  // namespace

void GetMinimalFov(const gfx::Transform& view_matrix,
                   const std::vector<const UiElement*>& elements,
                   const gvr::Rectf& fov_recommended,
                   float z_near,
                   gvr::Rectf* out_fov) {
  // Calculate boundary of Z near plane in view space.
  float z_near_left = -z_near * std::tan(fov_recommended.left * M_PI / 180);
  float z_near_right = z_near * std::tan(fov_recommended.right * M_PI / 180);
  float z_near_bottom = -z_near * std::tan(fov_recommended.bottom * M_PI / 180);
  float z_near_top = z_near * std::tan(fov_recommended.top * M_PI / 180);

  float left = z_near_right;
  float right = z_near_left;
  float bottom = z_near_top;
  float top = z_near_bottom;

  bool has_visible_element = false;

  for (const auto* element : elements) {
    gfx::Point3F left_bottom{-0.5, -0.5, 0};
    gfx::Point3F left_top{-0.5, 0.5, 0};
    gfx::Point3F right_bottom{0.5, -0.5, 0};
    gfx::Point3F right_top{0.5, 0.5, 0};

    gfx::Transform transform = element->world_space_transform();
    transform.ConcatTransform(view_matrix);

    // Transform to view space.
    transform.TransformPoint(&left_bottom);
    transform.TransformPoint(&left_top);
    transform.TransformPoint(&right_bottom);
    transform.TransformPoint(&right_top);

    // Project point to Z near plane in view space.
    left_bottom.Scale(-z_near / left_bottom.z());
    left_top.Scale(-z_near / left_top.z());
    right_bottom.Scale(-z_near / right_bottom.z());
    right_top.Scale(-z_near / right_top.z());

    // Find bounding box on z near plane.
    float bounds_left = std::min(
        {left_bottom.x(), left_top.x(), right_bottom.x(), right_top.x()});
    float bounds_right = std::max(
        {left_bottom.x(), left_top.x(), right_bottom.x(), right_top.x()});
    float bounds_bottom = std::min(
        {left_bottom.y(), left_top.y(), right_bottom.y(), right_top.y()});
    float bounds_top = std::max(
        {left_bottom.y(), left_top.y(), right_bottom.y(), right_top.y()});

    // Ignore non visible elements.
    if (bounds_left >= z_near_right || bounds_right <= z_near_left ||
        bounds_bottom >= z_near_top || bounds_top <= z_near_bottom ||
        bounds_left == bounds_right || bounds_bottom == bounds_top) {
      continue;
    }

    // Clamp to Z near plane's boundary.
    bounds_left = Clamp(bounds_left, z_near_left, z_near_right);
    bounds_right = Clamp(bounds_right, z_near_left, z_near_right);
    bounds_bottom = Clamp(bounds_bottom, z_near_bottom, z_near_top);
    bounds_top = Clamp(bounds_top, z_near_bottom, z_near_top);

    left = std::min(bounds_left, left);
    right = std::max(bounds_right, right);
    bottom = std::min(bounds_bottom, bottom);
    top = std::max(bounds_top, top);
    has_visible_element = true;
  }

  if (!has_visible_element) {
    *out_fov = gvr::Rectf{0.f, 0.f, 0.f, 0.f};
    return;
  }

  // Add a small margin to fix occasional border clipping due to precision.
  const float margin = std::tan(kMargin) * z_near;
  left = std::max(left - margin, z_near_left);
  right = std::min(right + margin, z_near_right);
  bottom = std::max(bottom - margin, z_near_bottom);
  top = std::min(top + margin, z_near_top);

  float left_degrees = std::atan(-left / z_near) * 180 / M_PI;
  float right_degrees = std::atan(right / z_near) * 180 / M_PI;
  float bottom_degrees = std::atan(-bottom / z_near) * 180 / M_PI;
  float top_degrees = std::atan(top / z_near) * 180 / M_PI;

  *out_fov =
      gvr::Rectf{left_degrees, right_degrees, bottom_degrees, top_degrees};
}

void TransformToGvrMat(const gfx::Transform& in, gvr::Mat4f* out) {
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      out->m[i][j] = in.matrix().get(i, j);
    }
  }
}

void GvrMatToTransform(const gvr::Mat4f& in, gfx::Transform* out) {
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      out->matrix().set(i, j, in.m[i][j]);
    }
  }
}

}  // namespace vr
