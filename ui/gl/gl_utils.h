// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains some useful utilities for the ui/gl classes.

#ifndef UI_GL_GL_UTILS_H_
#define UI_GL_GL_UTILS_H_

#include "ui/gl/gl_export.h"

namespace gfx {
class ColorSpace;
}  // namespace gfx

namespace gl {
GL_EXPORT int GetGLColorSpace(const gfx::ColorSpace& color_space);
}  // namespace gl

#endif  // UI_GL_GL_UTILS_H_
