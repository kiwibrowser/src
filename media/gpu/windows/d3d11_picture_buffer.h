// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_GPU_WINDOWS_D3D11_PICTURE_BUFFER_H_
#define MEDIA_GPU_WINDOWS_D3D11_PICTURE_BUFFER_H_

#include <d3d11.h>
#include <dxva.h>
#include <wrl/client.h>

#include <vector>

#include "base/memory/ref_counted.h"

#include "gpu/command_buffer/service/texture_manager.h"
#include "gpu/ipc/service/command_buffer_stub.h"
#include "media/base/video_frame.h"
#include "media/gpu/media_gpu_export.h"
#include "media/video/picture.h"
#include "third_party/angle/include/EGL/egl.h"
#include "third_party/angle/include/EGL/eglext.h"
#include "ui/gl/gl_image.h"

namespace media {

// PictureBuffer that owns Chrome Textures to display it, and keep a reference
// to the D3D texture that backs the image.
//
// This is created and owned on the decoder thread.  While currently that's the
// gpu main thread, we still keep the decoder parts separate from the chrome GL
// parts, in case that changes.
//
// This is refcounted so that VideoFrame can hold onto it indirectly.  While
// the chrome Texture is sufficient to keep the pictures renderable, we still
// need to guarantee that the client has time to use the mailbox.  Once it
// does so, it would be fine if this were destroyed.  Technically, only the
// GpuResources have to be retained until the mailbox is used, but we just
// retain the whole thing.
class MEDIA_GPU_EXPORT D3D11PictureBuffer
    : public base::RefCountedThreadSafe<D3D11PictureBuffer> {
 public:
  using MailboxHolderArray = gpu::MailboxHolder[VideoFrame::kMaxPlanes];

  D3D11PictureBuffer(GLenum target, gfx::Size size, size_t level);

  bool Init(base::RepeatingCallback<gpu::CommandBufferStub*()> get_stub_cb,
            Microsoft::WRL::ComPtr<ID3D11VideoDevice> video_device,
            Microsoft::WRL::ComPtr<ID3D11Texture2D> texture,
            const GUID& decoder_guid,
            int textures_per_picture);

  const gfx::Size& size() const { return size_; }
  size_t level() const { return level_; }
  Microsoft::WRL::ComPtr<ID3D11Texture2D> texture() const { return texture_; }

  // Is this PictureBuffer backing a VideoFrame right now?
  bool in_client_use() const { return in_client_use_; }

  // Is this PictureBuffer holding an image that's in use by the decoder?
  bool in_picture_use() const { return in_picture_use_; }

  void set_in_client_use(bool use) { in_client_use_ = use; }
  void set_in_picture_use(bool use) { in_picture_use_ = use; }

  const Microsoft::WRL::ComPtr<ID3D11VideoDecoderOutputView>& output_view()
      const {
    return output_view_;
  }

  // Return the mailbox holders that can be used to create a VideoFrame for us.
  const MailboxHolderArray& mailbox_holders() const { return mailbox_holders_; }

  // Shouldn't be here, but simpler for now.
  base::TimeDelta timestamp_;

 private:
  ~D3D11PictureBuffer();
  friend class base::RefCountedThreadSafe<D3D11PictureBuffer>;

  GLenum target_;
  gfx::Size size_;
  bool in_picture_use_ = false;
  bool in_client_use_ = false;
  size_t level_;

  // TODO(liberato): I don't think that we need to remember |texture_|.  The
  // GLImage will do so, so it will last long enough for any VideoFrames that
  // reference it.
  Microsoft::WRL::ComPtr<ID3D11Texture2D> texture_;
  Microsoft::WRL::ComPtr<ID3D11VideoDecoderOutputView> output_view_;

  MailboxHolderArray mailbox_holders_;

  // Things that are to be accessed / freed only on the main thread.  In
  // addition to setting up the textures to render from a D3D11 texture,
  // these also hold the chrome GL Texture objects so that the client
  // can use the mailbox.
  class GpuResources {
   public:
    GpuResources();
    ~GpuResources();

    bool Init(base::RepeatingCallback<gpu::CommandBufferStub*()> get_stub_cb,
              int level,
              const std::vector<gpu::Mailbox> mailboxes,
              GLenum target,
              gfx::Size size,
              Microsoft::WRL::ComPtr<ID3D11Texture2D> angle_texture,
              int textures_per_picture);

    std::vector<scoped_refptr<gpu::gles2::TextureRef>> texture_refs_;

   private:
    DISALLOW_COPY_AND_ASSIGN(GpuResources);
  };

  std::unique_ptr<GpuResources> gpu_resources_;

  DISALLOW_COPY_AND_ASSIGN(D3D11PictureBuffer);
};

}  // namespace media

#endif  // MEDIA_GPU_WINDOWS_D3D11_PICTURE_BUFFER_H_
