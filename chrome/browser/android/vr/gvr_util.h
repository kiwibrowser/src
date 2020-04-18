// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_VR_GVR_UTIL_H_
#define CHROME_BROWSER_ANDROID_VR_GVR_UTIL_H_

#include <vector>

#include "third_party/gvr-android-sdk/src/libraries/headers/vr/gvr/capi/include/gvr_types.h"

namespace gfx {
class Transform;
}  // namespace gfx

// Functions in this file are currently GVR specific functions. If other
// platforms need the same function here, please move it to
// chrome/browser/vr/*util.cc|h and remove dependancy to GVR.
namespace vr {

class UiElement;

// This function calculates the minimal FOV (in degrees) which covers all
// visible |elements| as if it was viewing from fov_recommended. For example, if
// fov_recommended is {20.f, 20.f, 20.f, 20.f}. And all elements appear on
// screen within a FOV of {-11.f, 19.f, 9.f, 9.f} if we use fov_recommended.
// Ideally, the calculated minimal FOV should be the same. In practice, the
// elements might get clipped near the edge sometimes due to float precison.
// To fix this, we add a small margin (1 degree) to all directions. So the
// |out_fov| set by this function should be {-10.f, 20.f, 10.f, 10.f} in the
// example case.
// Using a smaller FOV could improve the performance a lot while we are showing
// UIs on top of WebVR content.
void GetMinimalFov(const gfx::Transform& view_matrix,
                   const std::vector<const UiElement*>& elements,
                   const gvr::Rectf& fov_recommended,
                   float z_near,
                   gvr::Rectf* out_fov);

// Transforms the given gfx::Transform to gvr::Mat4f.
void TransformToGvrMat(const gfx::Transform& in, gvr::Mat4f* out);

// Transforms the given Mat4f to gfx::Transform.
void GvrMatToTransform(const gvr::Mat4f& in, gfx::Transform* out);

}  // namespace vr

#endif  // CHROME_BROWSER_ANDROID_VR_GVR_UTIL_H_
