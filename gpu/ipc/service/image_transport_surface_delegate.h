// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_IPC_SERVICE_IMAGE_TRANSPORT_SURFACE_DELEGATE_H_
#define GPU_IPC_SERVICE_IMAGE_TRANSPORT_SURFACE_DELEGATE_H_

#include "base/callback.h"
#include "gpu/command_buffer/common/texture_in_use_response.h"
#include "gpu/ipc/common/surface_handle.h"
#include "gpu/ipc/service/gpu_ipc_service_export.h"

namespace IPC {
class MessageFilter;
}

namespace gfx {
struct PresentationFeedback;
}

namespace gpu {
struct GpuPreferences;
struct SwapBuffersCompleteParams;

namespace gles2 {
class FeatureInfo;
}

class GPU_IPC_SERVICE_EXPORT ImageTransportSurfaceDelegate {
 public:
#if defined(OS_WIN)
  // Tells the delegate that a child window was created with the provided
  // SurfaceHandle.
  virtual void DidCreateAcceleratedSurfaceChildWindow(
      SurfaceHandle parent_window,
      SurfaceHandle child_window) = 0;
#endif

  // Tells the delegate that SwapBuffers returned.
  virtual void DidSwapBuffersComplete(SwapBuffersCompleteParams params) = 0;

  // Returns the features available for the ContextGroup.
  virtual const gles2::FeatureInfo* GetFeatureInfo() const = 0;

  virtual const GpuPreferences& GetGpuPreferences() const = 0;

  virtual void SetSnapshotRequestedCallback(const base::Closure& callback) = 0;

  // Tells the delegate a buffer has been presented.
  virtual void BufferPresented(const gfx::PresentationFeedback& feedback) = 0;
  // Add IPC message filter.
  virtual void AddFilter(IPC::MessageFilter* message_filter) = 0;
  // Gets route ID for sending / receiving IPC messages.
  virtual int32_t GetRouteID() const = 0;

 protected:
  virtual ~ImageTransportSurfaceDelegate() = default;
};

}  // namespace gpu

#endif  // GPU_IPC_SERVICE_IMAGE_TRANSPORT_SURFACE_DELEGATE_H_
