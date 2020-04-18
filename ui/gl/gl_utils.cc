// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains some useful utilities for the ui/gl classes.

#include "ui/gl/gl_utils.h"

#include "ui/gfx/color_space.h"
#include "ui/gl/gl_bindings.h"

namespace gl {

int GetGLColorSpace(const gfx::ColorSpace& color_space) {
  if (color_space.IsHDR())
    return GL_COLOR_SPACE_SCRGB_LINEAR_CHROMIUM;
  return GL_COLOR_SPACE_UNSPECIFIED_CHROMIUM;
}
}
