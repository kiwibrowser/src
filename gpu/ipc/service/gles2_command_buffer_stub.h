// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_IPC_SERVICE_GLES2_COMMAND_BUFFER_STUB_H_
#define GPU_IPC_SERVICE_GLES2_COMMAND_BUFFER_STUB_H_

#include "base/memory/weak_ptr.h"
#include "build/build_config.h"
#include "gpu/ipc/service/command_buffer_stub.h"
#include "gpu/ipc/service/image_transport_surface_delegate.h"

namespace gpu {

class GPU_IPC_SERVICE_EXPORT GLES2CommandBufferStub
    : public CommandBufferStub,
      public ImageTransportSurfaceDelegate,
      public base::SupportsWeakPtr<GLES2CommandBufferStub> {
 public:
  GLES2CommandBufferStub(GpuChannel* channel,
                         const GPUCreateCommandBufferConfig& init_params,
                         CommandBufferId command_buffer_id,
                         SequenceId sequence_id,
                         int32_t stream_id,
                         int32_t route_id);

  ~GLES2CommandBufferStub() override;

  // This must leave the GL context associated with the newly-created
  // CommandBufferStub current, so the GpuChannel can initialize
  // the gpu::Capabilities.
  gpu::ContextResult Initialize(
      CommandBufferStub* share_group,
      const GPUCreateCommandBufferConfig& init_params,
      std::unique_ptr<base::SharedMemory> shared_state_shm) override;

// ImageTransportSurfaceDelegate implementation:
#if defined(OS_WIN)
  void DidCreateAcceleratedSurfaceChildWindow(
      SurfaceHandle parent_window,
      SurfaceHandle child_window) override;
#endif
  void DidSwapBuffersComplete(SwapBuffersCompleteParams params) override;
  const gles2::FeatureInfo* GetFeatureInfo() const override;
  const GpuPreferences& GetGpuPreferences() const override;
  void SetSnapshotRequestedCallback(const base::Closure& callback) override;
  void BufferPresented(const gfx::PresentationFeedback& feedback) override;

  void AddFilter(IPC::MessageFilter* message_filter) override;
  int32_t GetRouteID() const override;

 private:
  void OnTakeFrontBuffer(const Mailbox& mailbox) override;
  void OnReturnFrontBuffer(const Mailbox& mailbox, bool is_lost) override;
  void OnSwapBuffers(uint64_t swap_id, uint32_t flags) override;

  // Keep a more specifically typed reference to the decoder to avoid
  // unnecessary casts. Owned by parent class.
  gles2::GLES2Decoder* gles2_decoder_;

  base::Closure snapshot_requested_callback_;

  // Params pushed each time we call OnSwapBuffers, and popped when a buffer
  // is presented or a swap completed.
  struct SwapBufferParams {
    uint64_t swap_id;
    uint32_t flags;
  };
  base::circular_deque<SwapBufferParams> pending_presented_params_;
  base::circular_deque<SwapBufferParams> pending_swap_completed_params_;

  base::WeakPtrFactory<GLES2CommandBufferStub> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(GLES2CommandBufferStub);
};

}  // namespace gpu

#endif  // GPU_IPC_SERVICE_GLES2_COMMAND_BUFFER_STUB_H_
