// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_IPC_RASTER_IN_PROCESS_CONTEXT_H_
#define GPU_IPC_RASTER_IN_PROCESS_CONTEXT_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "base/single_thread_task_runner.h"
#include "gpu/ipc/in_process_command_buffer.h"

namespace gpu {
class CommandBufferHelper;
class ContextSupport;
class ServiceTransferCache;
class TransferBuffer;
struct GpuFeatureInfo;
struct SharedMemoryLimits;

namespace raster {
class RasterInterface;
class RasterImplementation;
}  // namespace raster

// Runs client and server side command buffer code in process. Only supports
// RasterInterface.
class RasterInProcessContext {
 public:
  RasterInProcessContext();
  ~RasterInProcessContext();

  // |attrib_list| must be null or a NONE-terminated list of attribute/value
  // pairs. |gpu_channel_manager| should be non-null when used in the GPU
  // process.
  ContextResult Initialize(
      scoped_refptr<InProcessCommandBuffer::Service> service,
      const ContextCreationAttribs& attribs,
      const SharedMemoryLimits& memory_limits,
      GpuMemoryBufferManager* gpu_memory_buffer_manager,
      ImageFactory* image_factory,
      GpuChannelManagerDelegate* gpu_channel_manager_delegate,
      scoped_refptr<base::SingleThreadTaskRunner> task_runner);

  const Capabilities& GetCapabilities() const;
  const GpuFeatureInfo& GetGpuFeatureInfo() const;

  // Allows direct access to the RasterImplementation so a
  // RasterInProcessContext can be used without making it current.
  raster::RasterInterface* GetImplementation();

  ContextSupport* GetContextSupport();

  // Test only functions.
  ServiceTransferCache* GetTransferCacheForTest() const;

 private:
  std::unique_ptr<CommandBufferHelper> helper_;
  std::unique_ptr<TransferBuffer> transfer_buffer_;
  std::unique_ptr<raster::RasterImplementation> raster_implementation_;
  std::unique_ptr<InProcessCommandBuffer> command_buffer_;

  DISALLOW_COPY_AND_ASSIGN(RasterInProcessContext);
};

}  // namespace gpu

#endif  // GPU_IPC_RASTER_IN_PROCESS_CONTEXT_H_
