// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_TESTAPP_TEST_KEYBOARD_RENDERER_H_
#define CHROME_BROWSER_VR_TESTAPP_TEST_KEYBOARD_RENDERER_H_

#include "base/macros.h"
#include "third_party/skia/include/core/SkSurface.h"
#include "ui/gfx/geometry/size_f.h"
#include "ui/gl/gl_bindings.h"

namespace gfx {
class Transform;
}  // namespace gfx

namespace vr {

class SkiaSurfaceProvider;
class UiElementRenderer;
struct CameraModel;

class TestKeyboardRenderer {
 public:
  TestKeyboardRenderer();
  ~TestKeyboardRenderer();

  void Initialize(SkiaSurfaceProvider* provider, UiElementRenderer* renderer);
  void Draw(const CameraModel& model,
            const gfx::Transform& world_space_transform);

 private:
  GLuint texture_handle_;
  sk_sp<SkSurface> surface_;
  gfx::Size drawn_size_;
  UiElementRenderer* renderer_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(TestKeyboardRenderer);
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_TESTAPP_TEST_KEYBOARD_RENDERER_H_
