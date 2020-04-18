// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_TEST_GL_TEST_ENVIRONMENT_H_
#define CHROME_BROWSER_VR_TEST_GL_TEST_ENVIRONMENT_H_

#include "ui/gfx/geometry/size.h"
#include "ui/gl/gl_bindings.h"
#include "ui/gl/gl_context.h"
#include "ui/gl/gl_surface.h"

namespace vr {

class GlTestEnvironment {
 public:
  explicit GlTestEnvironment(const gfx::Size frame_buffer_size);
  ~GlTestEnvironment();

  GLuint GetFrameBufferForTesting();

 private:
  scoped_refptr<gl::GLSurface> surface_;
  scoped_refptr<gl::GLContext> context_;
  GLuint vao_ = 0;
  GLuint frame_buffer_ = 0;
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_TEST_GL_TEST_ENVIRONMENT_H_
