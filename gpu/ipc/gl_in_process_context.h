// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_IPC_GL_IN_PROCESS_CONTEXT_H_
#define GPU_IPC_GL_IN_PROCESS_CONTEXT_H_

#include <memory>

#include "base/single_thread_task_runner.h"
#include "gpu/command_buffer/common/context_creation_attribs.h"
#include "gpu/ipc/gl_in_process_context_export.h"
#include "gpu/ipc/in_process_command_buffer.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/gl/gl_surface.h"

namespace gpu {
struct GpuFeatureInfo;
struct SharedMemoryLimits;

namespace gles2 {
class GLES2Implementation;
}

class GL_IN_PROCESS_CONTEXT_EXPORT GLInProcessContext {
 public:
  virtual ~GLInProcessContext() = default;

  // TODO(danakj): Devirtualize this class and remove this, just call the
  // constructor.
  static std::unique_ptr<GLInProcessContext> CreateWithoutInit();

  // Initialize the GLInProcessContext, if |is_offscreen| is true, renders to an
  // offscreen context. |attrib_list| must be null or a NONE-terminated list
  // of attribute/value pairs.
  // If |surface| is not null, then it must match |is_offscreen|,
  // |window| must be gfx::kNullAcceleratedWidget, and the command buffer
  // service must run on the same thread as this client because GLSurface is
  // not thread safe. If |surface| is null, then the other parameters are used
  // to correctly create a surface.
  // |gpu_channel_manager| should be non-null when used in the GPU process.
  virtual ContextResult Initialize(
      scoped_refptr<InProcessCommandBuffer::Service> service,
      scoped_refptr<gl::GLSurface> surface,
      bool is_offscreen,
      SurfaceHandle window,
      const ContextCreationAttribs& attribs,
      const SharedMemoryLimits& memory_limits,
      GpuMemoryBufferManager* gpu_memory_buffer_manager,
      ImageFactory* image_factory,
      GpuChannelManagerDelegate* gpu_channel_manager_delegate,
      scoped_refptr<base::SingleThreadTaskRunner> task_runner) = 0;

  virtual const Capabilities& GetCapabilities() const = 0;
  virtual const GpuFeatureInfo& GetGpuFeatureInfo() const = 0;

  // Allows direct access to the GLES2 implementation so a GLInProcessContext
  // can be used without making it current.
  virtual gles2::GLES2Implementation* GetImplementation() = 0;

  virtual void SetLock(base::Lock* lock) = 0;

  virtual void SetUpdateVSyncParametersCallback(
      const InProcessCommandBuffer::UpdateVSyncParametersCallback&
          callback) = 0;
};

}  // namespace gpu

#endif  // GPU_IPC_GL_IN_PROCESS_CONTEXT_H_
