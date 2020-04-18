// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/vr_gl_util.h"

#include "ui/gfx/geometry/point3_f.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/gfx/geometry/size_f.h"
#include "ui/gfx/transform.h"

namespace vr {

// This code is adapted from the GVR Treasure Hunt demo source.
std::array<float, 16> MatrixToGLArray(const gfx::Transform& transform) {
  std::array<float, 16> result;
  transform.matrix().asColMajorf(result.data());
  return result;
}

// This code is adapted from the GVR Treasure Hunt demo source.
gfx::Rect CalculatePixelSpaceRect(const gfx::Size& texture_size,
                                  const gfx::RectF& texture_rect) {
  const gfx::RectF rect =
      ScaleRect(texture_rect, static_cast<float>(texture_size.width()),
                static_cast<float>(texture_size.height()));
  return gfx::Rect(rect.x(), rect.y(), rect.width(), rect.height());
}

GLuint CompileShader(GLenum shader_type,
                     const GLchar* shader_source,
                     std::string& error) {
  GLuint shader_handle = glCreateShader(shader_type);
  if (shader_handle != 0) {
    // Pass in the shader source.
    int len = strlen(shader_source);
    glShaderSource(shader_handle, 1, &shader_source, &len);
    // Compile the shader.
    glCompileShader(shader_handle);
    // Get the compilation status.
    GLint status;
    glGetShaderiv(shader_handle, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
      GLint info_log_length;
      glGetShaderiv(shader_handle, GL_INFO_LOG_LENGTH, &info_log_length);
      GLchar* str_info_log = new GLchar[info_log_length + 1];
      glGetShaderInfoLog(shader_handle, info_log_length, nullptr, str_info_log);
      error = "Error compiling shader: ";
      error += str_info_log;
      delete[] str_info_log;
      glDeleteShader(shader_handle);
      shader_handle = 0;
    }
  } else {
    error = "Could not create a shader handle (did not attempt compilation).";
  }

  return shader_handle;
}

GLuint CreateAndLinkProgram(GLuint vertext_shader_handle,
                            GLuint fragment_shader_handle,
                            std::string& error) {
  GLuint program_handle = glCreateProgram();

  if (program_handle != 0) {
    // Bind the vertex shader to the program.
    glAttachShader(program_handle, vertext_shader_handle);

    // Bind the fragment shader to the program.
    glAttachShader(program_handle, fragment_shader_handle);

    // Link the two shaders together into a program.
    glLinkProgram(program_handle);

    // Get the link status.
    GLint link_status;
    glGetProgramiv(program_handle, GL_LINK_STATUS, &link_status);

    // If the link failed, delete the program.
    if (link_status == GL_FALSE) {
      GLint info_log_length;
      glGetProgramiv(program_handle, GL_INFO_LOG_LENGTH, &info_log_length);

      GLchar* str_info_log = new GLchar[info_log_length + 1];
      glGetProgramInfoLog(program_handle, info_log_length, nullptr,
                          str_info_log);
      error = "Error compiling program: ";
      error += str_info_log;
      delete[] str_info_log;
      glDeleteProgram(program_handle);
      program_handle = 0;
    }
  }

  return program_handle;
}

gfx::SizeF CalculateScreenSize(const gfx::Transform& proj_matrix,
                               float distance,
                               const gfx::SizeF& size) {
  // View matrix is the identity, thus, not needed in the calculation.
  gfx::Transform scale_transform;
  scale_transform.Scale(size.width(), size.height());

  gfx::Transform translate_transform;
  translate_transform.Translate3d(0, 0, -distance);

  gfx::Transform model_view_proj_matrix =
      proj_matrix * translate_transform * scale_transform;

  gfx::Point3F projected_upper_right_corner(0.5f, 0.5f, 0.0f);
  model_view_proj_matrix.TransformPoint(&projected_upper_right_corner);
  gfx::Point3F projected_lower_left_corner(-0.5f, -0.5f, 0.0f);
  model_view_proj_matrix.TransformPoint(&projected_lower_left_corner);

  // Calculate and return the normalized size in screen space.
  return gfx::SizeF((std::abs(projected_upper_right_corner.x()) +
                     std::abs(projected_lower_left_corner.x())) /
                        2.0f,
                    (std::abs(projected_upper_right_corner.y()) +
                     std::abs(projected_lower_left_corner.y())) /
                        2.0f);
}

void SetTexParameters(GLenum texture_type) {
  glTexParameteri(texture_type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(texture_type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(texture_type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(texture_type, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

void SetColorUniform(GLuint handle, SkColor c) {
  glUniform4f(handle, SkColorGetR(c) / 255.0, SkColorGetG(c) / 255.0,
              SkColorGetB(c) / 255.0, SkColorGetA(c) / 255.0);
}

void SetOpaqueColorUniform(GLuint handle, SkColor c) {
  glUniform3f(handle, SkColorGetR(c) / 255.0, SkColorGetG(c) / 255.0,
              SkColorGetB(c) / 255.0);
}

}  // namespace vr
