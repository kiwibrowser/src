// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_VR_ARCORE_DEVICE_ARCORE_H_
#define CHROME_BROWSER_ANDROID_VR_ARCORE_DEVICE_ARCORE_H_

#include <memory>
#include <vector>
#include "base/macros.h"
#include "device/vr/public/mojom/vr_service.mojom.h"
#include "ui/display/display.h"
#include "ui/gfx/transform.h"
#include "ui/gl/gl_bindings.h"

namespace device {

// This allows a real or fake implementation of ARCore to
// be used as appropriate (i.e. for testing).
class ARCore {
 public:
  virtual ~ARCore() = default;

  virtual bool Initialize() = 0;

  virtual void SetDisplayGeometry(
      const gfx::Size& frame_size,
      display::Display::Rotation display_rotation) = 0;
  virtual void SetCameraTexture(GLuint camera_texture_id) = 0;
  // Transform the given UV coordinates by the current display rotation.
  virtual std::vector<float> TransformDisplayUvCoords(
      const base::span<const float> uvs) = 0;
  virtual gfx::Transform GetProjectionMatrix(float near, float far) = 0;
  virtual mojom::VRPosePtr Update() = 0;
};

}  // namespace device

#endif  // CHROME_BROWSER_ANDROID_VR_ARCORE_DEVICE_ARCORE_H_
