// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GL_GL_IMPLEMENTATION_OSMESA_H_
#define UI_GL_GL_IMPLEMENTATION_OSMESA_H_

#include "base/files/file_path.h"
#include "base/native_library.h"
#include "ui/gl/gl_export.h"

namespace gl {

GL_EXPORT bool InitializeStaticGLBindingsOSMesaGL();

}  // namespace gl

#endif  // UI_GL_GL_IMPLEMENTATION_OSMESA_H_
