// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_COMMAND_BUFFER_SERVICE_GL_CONTEXT_VIRTUAL_H_
#define GPU_COMMAND_BUFFER_SERVICE_GL_CONTEXT_VIRTUAL_H_

#include <string>
#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "gpu/gpu_gles2_export.h"
#include "ui/gl/gl_context.h"

namespace gl {
class GPUTimingClient;
class GLShareGroup;
class GLSurface;
}

namespace gpu {
class DecoderContext;

// Encapsulates a virtual OpenGL context.
class GPU_GLES2_EXPORT GLContextVirtual : public gl::GLContext {
 public:
  GLContextVirtual(gl::GLShareGroup* share_group,
                   gl::GLContext* shared_context,
                   base::WeakPtr<DecoderContext> decoder);

  // Implement GLContext.
  bool Initialize(gl::GLSurface* compatible_surface,
                  const gl::GLContextAttribs& attribs) override;
  bool MakeCurrent(gl::GLSurface* surface) override;
  void ReleaseCurrent(gl::GLSurface* surface) override;
  bool IsCurrent(gl::GLSurface* surface) override;
  void* GetHandle() override;
  scoped_refptr<gl::GPUTimingClient> CreateGPUTimingClient() override;
  std::string GetGLVersion() override;
  std::string GetGLRenderer() override;
  const gl::ExtensionSet& GetExtensions() override;
  void SetSafeToForceGpuSwitch() override;
  bool WasAllocatedUsingRobustnessExtension() override;
  void SetUnbindFboOnMakeCurrent() override;
  gl::YUVToRGBConverter* GetYUVToRGBConverter(
      const gfx::ColorSpace& color_space) override;
  void ForceReleaseVirtuallyCurrent() override;

 protected:
  ~GLContextVirtual() override;
  void ResetExtensions() override;

 private:
  void Destroy();

  scoped_refptr<gl::GLContext> shared_context_;
  base::WeakPtr<DecoderContext> decoder_;

  DISALLOW_COPY_AND_ASSIGN(GLContextVirtual);
};

}  // namespace gpu

#endif  // GPU_COMMAND_BUFFER_SERVICE_GL_CONTEXT_VIRTUAL_H_
