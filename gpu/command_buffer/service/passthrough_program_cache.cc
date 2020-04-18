// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/service/passthrough_program_cache.h"

#include <stddef.h>

#include "base/base64.h"
#include "ui/gl/gl_bindings.h"

#if defined(USE_EGL)
#include "ui/gl/angle_platform_impl.h"
#include "ui/gl/gl_surface_egl.h"
#endif  // defined(USE_EGL)

#ifndef EGL_ANGLE_program_cache_control
#define EGL_ANGLE_program_cache_control 1
#define EGL_PROGRAM_CACHE_SIZE_ANGLE 0x3455
#define EGL_PROGRAM_CACHE_KEY_LENGTH_ANGLE 0x3456
#define EGL_PROGRAM_CACHE_RESIZE_ANGLE 0x3457
#define EGL_PROGRAM_CACHE_TRIM_ANGLE 0x3458
#define EGL_CONTEXT_PROGRAM_BINARY_CACHE_ENABLED_ANGLE 0x3459
#endif /* EGL_ANGLE_program_cache_control */

namespace gpu {
namespace gles2 {

namespace {

bool ProgramCacheControlExtensionAvailable() {
#if defined(USE_EGL)
  // The display should be initialized if the extension is available.
  return gl::g_driver_egl.ext.b_EGL_ANGLE_program_cache_control;
#else
  return false;
#endif  // defined(USE_EGL)
}

}  // namespace

PassthroughProgramCache::PassthroughProgramCache(
    size_t max_cache_size_bytes,
    bool disable_gpu_shader_disk_cache)
    : ProgramCache(max_cache_size_bytes),
      disable_gpu_shader_disk_cache_(disable_gpu_shader_disk_cache) {
  if (!CacheEnabled()) {
    return;
  }

#if defined(USE_EGL)
  EGLDisplay display = gl::GLSurfaceEGL::GetHardwareDisplay();
  DCHECK(display != EGL_NO_DISPLAY);

  eglProgramCacheResizeANGLE(display, max_cache_size_bytes,
                             EGL_PROGRAM_CACHE_RESIZE_ANGLE);
#endif  // defined(USE_EGL)
}

PassthroughProgramCache::~PassthroughProgramCache() {
#if defined(USE_EGL)
  // Ensure the program cache callback is cleared.
  angle::ResetCacheProgramCallback();
#endif  // defined(USE_EGL)
}

void PassthroughProgramCache::ClearBackend() {
  Trim(0);
}

ProgramCache::ProgramLoadResult PassthroughProgramCache::LoadLinkedProgram(
    GLuint program,
    Shader* shader_a,
    Shader* shader_b,
    const LocationMap* bind_attrib_location_map,
    const std::vector<std::string>& transform_feedback_varyings,
    GLenum transform_feedback_buffer_mode,
    DecoderClient* client) {
  NOTREACHED();
  return PROGRAM_LOAD_FAILURE;
}

void PassthroughProgramCache::SaveLinkedProgram(
    GLuint program,
    const Shader* shader_a,
    const Shader* shader_b,
    const LocationMap* bind_attrib_location_map,
    const std::vector<std::string>& transform_feedback_varyings,
    GLenum transform_feedback_buffer_mode,
    DecoderClient* client) {
  NOTREACHED();
}

void PassthroughProgramCache::LoadProgram(const std::string& key,
                                          const std::string& program) {
  if (!CacheEnabled()) {
    // Early exit if this display can't support cache control
    return;
  }

#if defined(USE_EGL)
  EGLDisplay display = gl::GLSurfaceEGL::GetHardwareDisplay();
  DCHECK(display != EGL_NO_DISPLAY);

  std::string key_decoded;
  std::string program_decoded;
  base::Base64Decode(key, &key_decoded);
  base::Base64Decode(program, &program_decoded);

  eglProgramCachePopulateANGLE(display, key_decoded.c_str(), key_decoded.size(),
                               program_decoded.c_str(), program_decoded.size());
#endif  // defined(USE_EGL)
}

size_t PassthroughProgramCache::Trim(size_t limit) {
  if (!CacheEnabled()) {
    // Early exit if this display can't support cache control
    return 0;
  }

#if defined(USE_EGL)
  EGLDisplay display = gl::GLSurfaceEGL::GetHardwareDisplay();
  DCHECK(display != EGL_NO_DISPLAY);

  EGLint trimmed =
      eglProgramCacheResizeANGLE(display, limit, EGL_PROGRAM_CACHE_TRIM_ANGLE);
  return static_cast<size_t>(trimmed);
#else
  return 0;
#endif  // defined(USE_EGL)
}

bool PassthroughProgramCache::CacheEnabled() const {
  return ProgramCacheControlExtensionAvailable() &&
         !disable_gpu_shader_disk_cache_;
}

}  // namespace gles2
}  // namespace gpu
