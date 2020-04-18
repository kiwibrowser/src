// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/ipc/raster_in_process_context.h"

#include <utility>

#include "base/command_line.h"
#include "base/logging.h"
#include "gpu/command_buffer/client/gles2_cmd_helper.h"
#include "gpu/command_buffer/client/raster_cmd_helper.h"
#include "gpu/command_buffer/client/raster_implementation.h"
#include "gpu/command_buffer/client/raster_implementation_gles.h"
#include "gpu/command_buffer/client/shared_memory_limits.h"
#include "gpu/command_buffer/client/transfer_buffer.h"
#include "gpu/command_buffer/common/command_buffer.h"
#include "gpu/command_buffer/common/constants.h"
#include "gpu/command_buffer/common/context_creation_attribs.h"
#include "gpu/config/gpu_feature_info.h"
#include "gpu/config/gpu_switches.h"
#include "gpu/ipc/common/surface_handle.h"

namespace gpu {

RasterInProcessContext::RasterInProcessContext() = default;

RasterInProcessContext::~RasterInProcessContext() {
  // First flush the contexts to ensure that any pending frees of resources
  // are completed. Otherwise, if this context is part of a share group,
  // those resources might leak. Also, any remaining side effects of commands
  // issued on this context might not be visible to other contexts in the
  // share group.
  if (raster_implementation_) {
    raster_implementation_->Flush();
    raster_implementation_.reset();
  }
  transfer_buffer_.reset();
  helper_.reset();
  command_buffer_.reset();
}

ContextResult RasterInProcessContext::Initialize(
    scoped_refptr<InProcessCommandBuffer::Service> service,
    const ContextCreationAttribs& attribs,
    const SharedMemoryLimits& memory_limits,
    GpuMemoryBufferManager* gpu_memory_buffer_manager,
    ImageFactory* image_factory,
    GpuChannelManagerDelegate* gpu_channel_manager_delegate,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner) {
  DCHECK(attribs.enable_raster_interface);
  if (!attribs.enable_raster_interface) {
    return ContextResult::kFatalFailure;
  }
  DCHECK(!attribs.enable_gles2_interface);
  if (attribs.enable_gles2_interface) {
    return ContextResult::kFatalFailure;
  }

  // TODO(backer): Remove this. Currently used to set
  // |chromium_raster_transport| features flag (https://crbug.com/786591) and
  // enable_oop_rasterization in GpuPreferences (https://crbug.com/829469).
  if (attribs.enable_oop_rasterization &&
      !base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kEnableOOPRasterization)) {
    base::CommandLine::ForCurrentProcess()->AppendSwitch(
        switches::kEnableOOPRasterization);
  }

  command_buffer_ = std::make_unique<InProcessCommandBuffer>(service);
  auto result = command_buffer_->Initialize(
      nullptr /* surface */, true /* is_offscreen */, kNullSurfaceHandle,
      attribs, nullptr /* share_command_buffer */, gpu_memory_buffer_manager,
      image_factory, gpu_channel_manager_delegate, std::move(task_runner));
  if (result != ContextResult::kSuccess) {
    DLOG(ERROR) << "Failed to initialize InProcessCommmandBuffer";
    return result;
  }

  // Check for consistency.
  DCHECK(!attribs.bind_generates_resource);
  constexpr bool bind_generates_resource = false;

  // TODO(https://crbug.com/829469): Remove check once we fuzz RasterDecoder.
  // enable_oop_rasterization is currently necessary to create RasterDecoder
  // in InProcessCommandBuffer.
  DCHECK(attribs.enable_oop_rasterization);

  // Create the RasterCmdHelper, which writes the command buffer protocol.
  auto raster_helper =
      std::make_unique<raster::RasterCmdHelper>(command_buffer_.get());
  result = raster_helper->Initialize(memory_limits.command_buffer_size);
  if (result != ContextResult::kSuccess) {
    LOG(ERROR) << "Failed to initialize RasterCmdHelper";
    return result;
  }
  transfer_buffer_ = std::make_unique<TransferBuffer>(raster_helper.get());

  auto raster_implementation = std::make_unique<raster::RasterImplementation>(
      raster_helper.get(), transfer_buffer_.get(), bind_generates_resource,
      attribs.lose_context_when_out_of_memory, command_buffer_.get());
  result = raster_implementation->Initialize(memory_limits);
  raster_implementation_ = std::move(raster_implementation);
  helper_ = std::move(raster_helper);
  return result;
}

const Capabilities& RasterInProcessContext::GetCapabilities() const {
  return command_buffer_->GetCapabilities();
}

const GpuFeatureInfo& RasterInProcessContext::GetGpuFeatureInfo() const {
  return command_buffer_->GetGpuFeatureInfo();
}

raster::RasterInterface* RasterInProcessContext::GetImplementation() {
  return raster_implementation_.get();
}

ContextSupport* RasterInProcessContext::GetContextSupport() {
  return raster_implementation_.get();
}

ServiceTransferCache* RasterInProcessContext::GetTransferCacheForTest() const {
  return command_buffer_->GetTransferCacheForTest();
}

}  // namespace gpu
