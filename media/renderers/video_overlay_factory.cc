// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/renderers/video_overlay_factory.h"

#include "base/single_thread_task_runner.h"
#include "gpu/GLES2/gl2extchromium.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "gpu/command_buffer/common/mailbox.h"
#include "gpu/command_buffer/common/sync_token.h"
#include "media/base/video_frame.h"
#include "media/video/gpu_video_accelerator_factories.h"

namespace media {

class VideoOverlayFactory::Texture {
 public:
  explicit Texture(GpuVideoAcceleratorFactories* gpu_factories)
      : gpu_factories_(gpu_factories), image_id_(0), texture_id_(0) {
    DCHECK(gpu_factories_);
    DCHECK(gpu_factories_->GetTaskRunner()->BelongsToCurrentThread());

    gpu::gles2::GLES2Interface* gl = gpu_factories_->ContextGL();
    if (!gl)
      return;
    gpu_memory_buffer_ = gpu_factories_->CreateGpuMemoryBuffer(
        gfx::Size(1, 1), gfx::BufferFormat::BGRA_8888,
        gfx::BufferUsage::SCANOUT);
    if (gpu_memory_buffer_) {
      image_id_ = gl->CreateImageCHROMIUM(gpu_memory_buffer_->AsClientBuffer(),
                                          1, 1, GL_BGRA_EXT);
      }
      if (image_id_) {
        gl->GenTextures(1, &texture_id_);
        gl->BindTexture(GL_TEXTURE_2D, texture_id_);
        gl->BindTexImage2DCHROMIUM(GL_TEXTURE_2D, image_id_);

        gl->GenMailboxCHROMIUM(mailbox_.name);
        gl->ProduceTextureDirectCHROMIUM(texture_id_, mailbox_.name);

        gl->GenSyncTokenCHROMIUM(sync_token_.GetData());
      }
  }

  ~Texture() {
    DCHECK(gpu_factories_->GetTaskRunner()->BelongsToCurrentThread());

    if (image_id_) {
      gpu::gles2::GLES2Interface* gl = gpu_factories_->ContextGL();
      if (!gl)
        return;
      gl->BindTexture(GL_TEXTURE_2D, texture_id_);
      gl->ReleaseTexImage2DCHROMIUM(GL_TEXTURE_2D, image_id_);
      gl->DeleteTextures(1, &texture_id_);
      gl->DestroyImageCHROMIUM(image_id_);
    }
  }

  bool IsValid() const { return image_id_ != 0; }

 private:
  friend class VideoOverlayFactory;
  GpuVideoAcceleratorFactories* gpu_factories_;

  std::unique_ptr<gfx::GpuMemoryBuffer> gpu_memory_buffer_;
  GLuint image_id_;
  GLuint texture_id_;
  gpu::Mailbox mailbox_;
  gpu::SyncToken sync_token_;
};

VideoOverlayFactory::VideoOverlayFactory(
    GpuVideoAcceleratorFactories* gpu_factories)
    : gpu_factories_(gpu_factories) {}

VideoOverlayFactory::~VideoOverlayFactory() = default;

scoped_refptr<VideoFrame> VideoOverlayFactory::CreateFrame(
    const gfx::Size& size) {
  // Frame size empty => video has one dimension = 0.
  // Dimension 0 case triggers a DCHECK later on if we push through the overlay
  // path.
  Texture* texture = size.IsEmpty() ? nullptr : GetTexture();
  if (!texture) {
    DVLOG(1) << "Create black frame " << size.width() << "x" << size.height();
    return VideoFrame::CreateBlackFrame(gfx::Size(1, 1));
  }

  DCHECK(texture);
  DCHECK(texture->IsValid());
  DVLOG(2) << "Create video overlay frame: " << size.ToString();
  gpu::MailboxHolder holders[VideoFrame::kMaxPlanes] = {gpu::MailboxHolder(
      texture->mailbox_, texture->sync_token_, GL_TEXTURE_2D)};
  scoped_refptr<VideoFrame> frame = VideoFrame::WrapNativeTextures(
      PIXEL_FORMAT_XRGB, holders, VideoFrame::ReleaseMailboxCB(),
      size,                // coded_size
      gfx::Rect(size),     // visible rect
      size,                // natural size
      base::TimeDelta());  // timestamp
  CHECK(frame);
  frame->metadata()->SetBoolean(VideoFrameMetadata::ALLOW_OVERLAY, true);
  return frame;
}

VideoOverlayFactory::Texture* VideoOverlayFactory::GetTexture() {
  if (!gpu_factories_)
    return nullptr;

  // Lazily create overlay texture.
  if (!texture_)
    texture_.reset(new Texture(gpu_factories_));

  return texture_->IsValid() ? texture_.get() : nullptr;
}

}  // namespace media
