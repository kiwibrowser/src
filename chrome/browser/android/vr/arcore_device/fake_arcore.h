// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_VR_ARCORE_DEVICE_FAKE_ARCORE_H_
#define CHROME_BROWSER_ANDROID_VR_ARCORE_DEVICE_FAKE_ARCORE_H_

#include <memory>
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/time/time.h"
#include "chrome/browser/android/vr/arcore_device/arcore.h"

namespace gl {
class GLImageAHardwareBuffer;
}  // namespace gl

namespace device {

// Minimal fake ARCore implementation for testing. It can populate
// the camera texture with a GL_TEXTURE_OES image and do UV transform
// calculations.
class FakeARCore : public ARCore {
 public:
  FakeARCore();
  ~FakeARCore() override;

  // ARCoreDriverBase implementation.
  bool Initialize() override;
  void SetCameraTexture(GLuint texture) override;
  void SetDisplayGeometry(const gfx::Size& frame_size,
                          display::Display::Rotation display_rotation) override;
  std::vector<float> TransformDisplayUvCoords(
      const base::span<const float> uvs) override;
  gfx::Transform GetProjectionMatrix(float near, float far) override;
  mojom::VRPosePtr Update() override;

  void SetCameraAspect(float aspect) { camera_aspect_ = aspect; }

 private:
  float camera_aspect_ = 1.0f;
  display::Display::Rotation display_rotation_ =
      display::Display::Rotation::ROTATE_0;
  gfx::Size frame_size_;
  // Storage for the testing placeholder image to keep it alive.
  scoped_refptr<gl::GLImageAHardwareBuffer> placeholder_camera_image_;

  DISALLOW_COPY_AND_ASSIGN(FakeARCore);
};

}  // namespace device

#endif  // CHROME_BROWSER_ANDROID_VR_ARCORE_DEVICE_FAKE_ARCORE_H_
