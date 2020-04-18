// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_VR_GL_UTIL_H_
#define CHROME_BROWSER_VR_VR_GL_UTIL_H_

#include <array>
#include <string>

#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gl/gl_bindings.h"

#define SHADER(Src) "#version 100\n" #Src
#define OEIE_SHADER(Src) \
  "#version 100\n#extension GL_OES_EGL_image_external : require\n" #Src
#define VOID_OFFSET(x) reinterpret_cast<void*>(x)

namespace gfx {
class RectF;
class Size;
class SizeF;
class Transform;
}  // namespace gfx

namespace vr {

std::array<float, 16> MatrixToGLArray(const gfx::Transform& matrix);

gfx::Rect CalculatePixelSpaceRect(const gfx::Size& texture_size,
                                  const gfx::RectF& texture_rect);

// Compile a shader.
GLuint CompileShader(GLenum shader_type,
                     const GLchar* shader_source,
                     std::string& error);

// Compile and link a program.
GLuint CreateAndLinkProgram(GLuint vertex_shader_handle,
                            GLuint fragment_shader_handle,
                            std::string& error);

// Returns the normalized size of the element projected into screen space.
// If (1, 1) the element fills the entire buffer.
gfx::SizeF CalculateScreenSize(const gfx::Transform& proj_matrix,
                               float distance,
                               const gfx::SizeF& size);

// Sets default texture parameters given a texture type.
void SetTexParameters(GLenum texture_type);

// Sets color uniforms given an SkColor.
void SetColorUniform(GLuint handle, SkColor c);

// Sets color uniforms (but not alpha) given an SkColor. The alpha is assumed to
// be 1.0 in this case.
void SetOpaqueColorUniform(GLuint handle, SkColor c);

}  // namespace vr

#endif  // CHROME_BROWSER_VR_VR_GL_UTIL_H_
