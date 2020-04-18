// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_IPC_SERVICE_STREAM_TEXTURE_ANDROID_H_
#define GPU_IPC_SERVICE_STREAM_TEXTURE_ANDROID_H_

#include <stdint.h>

#include <memory>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/unguessable_token.h"
#include "gpu/command_buffer/service/gl_stream_texture_image.h"
#include "gpu/ipc/service/command_buffer_stub.h"
#include "ipc/ipc_listener.h"
#include "ui/gl/android/surface_texture.h"
#include "ui/gl/gl_image.h"

namespace ui {
class ScopedMakeCurrent;
}

namespace gfx {
class Size;
}

namespace gpu {

class StreamTexture : public gpu::gles2::GLStreamTextureImage,
                      public IPC::Listener,
                      public CommandBufferStub::DestructionObserver {
 public:
  static bool Create(CommandBufferStub* owner_stub,
                     uint32_t client_texture_id,
                     int stream_id);

 private:
  StreamTexture(CommandBufferStub* owner_stub,
                int32_t route_id,
                uint32_t texture_id);
  ~StreamTexture() override;

  // gl::GLImage implementation:
  gfx::Size GetSize() override;
  unsigned GetInternalFormat() override;
  bool BindTexImage(unsigned target) override;
  void ReleaseTexImage(unsigned target) override;
  bool CopyTexImage(unsigned target) override;
  bool CopyTexSubImage(unsigned target,
                       const gfx::Point& offset,
                       const gfx::Rect& rect) override;
  bool ScheduleOverlayPlane(gfx::AcceleratedWidget widget,
                            int z_order,
                            gfx::OverlayTransform transform,
                            const gfx::Rect& bounds_rect,
                            const gfx::RectF& crop_rect,
                            bool enable_blend,
                            gfx::GpuFence* gpu_fence) override;
  void SetColorSpace(const gfx::ColorSpace& color_space) override {}
  void Flush() override {}
  void OnMemoryDump(base::trace_event::ProcessMemoryDump* pmd,
                    uint64_t process_tracing_id,
                    const std::string& dump_name) override;

  // gpu::gles2::GLStreamTextureMatrix implementation
  void GetTextureMatrix(float xform[16]) override;
  void NotifyPromotionHint(bool promotion_hint,
                           int display_x,
                           int display_y,
                           int display_width,
                           int display_height) override {}

  // CommandBufferStub::DestructionObserver implementation.
  void OnWillDestroyStub(bool have_context) override;

  std::unique_ptr<ui::ScopedMakeCurrent> MakeStubCurrent();

  void UpdateTexImage();

  // Called when a new frame is available for the SurfaceTexture.
  void OnFrameAvailable();

  // IPC::Listener implementation:
  bool OnMessageReceived(const IPC::Message& message) override;

  // IPC message handlers:
  void OnStartListening();
  void OnForwardForSurfaceRequest(const base::UnguessableToken& request_token);
  void OnSetSize(const gfx::Size& size) { size_ = size; }

  scoped_refptr<gl::SurfaceTexture> surface_texture_;

  // Current transform matrix of the surface texture.
  float current_matrix_[16];

  // Current size of the surface texture.
  gfx::Size size_;

  // Whether a new frame is available that we should update to.
  bool has_pending_frame_;

  CommandBufferStub* owner_stub_;
  int32_t route_id_;
  bool has_listener_;
  uint32_t texture_id_;

  base::WeakPtrFactory<StreamTexture> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(StreamTexture);
};

}  // namespace gpu

#endif  // GPU_IPC_SERVICE_STREAM_TEXTURE_ANDROID_H_
