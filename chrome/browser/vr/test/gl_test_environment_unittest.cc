// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/test/gl_test_environment.h"
#include "build/build_config.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gl/gl_bindings.h"

namespace vr {

TEST(GlTestEnvironmentTest, InitializeAndCleanup) {
// TODO(crbug/771794): Test temporarily disabled on Windows because it crashes
// on trybots. Fix before enabling Windows support.
#ifndef OS_WIN
  GlTestEnvironment gl_test_environment(gfx::Size(100, 100));
  EXPECT_NE(gl_test_environment.GetFrameBufferForTesting(), 0u);
  EXPECT_EQ(glGetError(), (GLenum)GL_NO_ERROR);
#endif
  // We just test that clean up doesn't crash.
}

}  // namespace vr
