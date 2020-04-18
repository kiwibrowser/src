// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GL_INIT_GL_INITIALIZER_H_
#define UI_GL_INIT_GL_INITIALIZER_H_

#include "ui/gl/gl_implementation.h"

namespace gl {
namespace init {

// Performs platform dependent one-off GL initialization, calling into the
// appropriate GLSurface code to initialize it. To perform one-off GL
// initialization you should use InitializeGLOneOff() or for tests possibly
// InitializeGLOneOffImplementation() instead.
bool InitializeGLOneOffPlatform();

// Initializes a particular GL implementation.
bool InitializeStaticGLBindings(GLImplementation implementation);

// Initializes debug logging wrappers for GL bindings.
void InitializeDebugGLBindings();

// Clears GL bindings for all implementations supported by platform.
void ShutdownGLPlatform();

}  // namespace init
}  // namespace gl

#endif  // UI_GL_INIT_GL_INITIALIZER_H_
