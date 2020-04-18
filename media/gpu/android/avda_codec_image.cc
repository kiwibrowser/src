// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/gpu/android/avda_codec_image.h"

#include <string.h>

#include <memory>

#include "gpu/command_buffer/service/texture_manager.h"
#include "media/base/android/media_codec_bridge_impl.h"
#include "media/gpu/android/avda_shared_state.h"
#include "ui/gl/android/surface_texture.h"
#include "ui/gl/gl_context.h"
#include "ui/gl/scoped_make_current.h"

namespace media {

AVDACodecImage::AVDACodecImage(
    const scoped_refptr<AVDASharedState>& shared_state,
    MediaCodecBridge* codec)
    : shared_state_(shared_state),
      codec_buffer_index_(kInvalidCodecBufferIndex),
      media_codec_(codec),
      has_texture_owner_(false),
      texture_(0) {}

AVDACodecImage::~AVDACodecImage() {}

gfx::Size AVDACodecImage::GetSize() {
  return size_;
}

unsigned AVDACodecImage::GetInternalFormat() {
  return GL_RGBA;
}

bool AVDACodecImage::BindTexImage(unsigned target) {
  return false;
}

void AVDACodecImage::ReleaseTexImage(unsigned target) {}

bool AVDACodecImage::CopyTexImage(unsigned target) {
  if (!has_texture_owner_ || target != GL_TEXTURE_EXTERNAL_OES)
    return false;

  GLint bound_service_id = 0;
  glGetIntegerv(GL_TEXTURE_BINDING_EXTERNAL_OES, &bound_service_id);
  // We insist that the currently bound texture is the right one.
  if (bound_service_id !=
      static_cast<GLint>(shared_state_->texture_owner_service_id())) {
    return false;
  }

  // Make sure that we have the right image in the front buffer.  Note that the
  // bound_service_id is guaranteed to be equal to the texture owner's client
  // texture id, so we can skip preserving it if the right context is current.
  UpdateSurfaceInternal(UpdateMode::RENDER_TO_FRONT_BUFFER,
                        kDontRestoreBindings);

  // By setting image state to UNBOUND instead of COPIED we ensure that
  // CopyTexImage() is called each time the texture owner is used for drawing.
  // It would be nice if we could do this via asking for the currently bound
  // Texture, but the active unit never seems to change.
  texture_->SetLevelImageState(GL_TEXTURE_EXTERNAL_OES, 0,
                               gpu::gles2::Texture::UNBOUND);

  return true;
}

bool AVDACodecImage::CopyTexSubImage(unsigned target,
                                     const gfx::Point& offset,
                                     const gfx::Rect& rect) {
  return false;
}

bool AVDACodecImage::ScheduleOverlayPlane(gfx::AcceleratedWidget widget,
                                          int z_order,
                                          gfx::OverlayTransform transform,
                                          const gfx::Rect& bounds_rect,
                                          const gfx::RectF& crop_rect,
                                          bool enable_blend,
                                          gfx::GpuFence* gpu_fence) {
  // This should only be called when we're rendering to a SurfaceView.
  if (has_texture_owner_) {
    DVLOG(1) << "Invalid call to ScheduleOverlayPlane; this image is "
                "TextureOwner backed.";
    return false;
  }

  // Move the overlay if needed.
  if (shared_state_->overlay() && most_recent_bounds_ != bounds_rect) {
    most_recent_bounds_ = bounds_rect;
    shared_state_->overlay()->ScheduleLayout(bounds_rect);
  }

  UpdateSurface(UpdateMode::RENDER_TO_FRONT_BUFFER);
  return true;
}

void AVDACodecImage::OnMemoryDump(base::trace_event::ProcessMemoryDump* pmd,
                                  uint64_t process_tracing_id,
                                  const std::string& dump_name) {}

void AVDACodecImage::UpdateTextureOwner(RestoreBindingsMode mode) {
  DCHECK(has_texture_owner_);
  DCHECK_EQ(codec_buffer_index_, kUpdateOnly);
  codec_buffer_index_ = kRendered;

  // Swap the rendered image to the front.
  std::unique_ptr<ui::ScopedMakeCurrent> scoped_make_current =
      MakeCurrentIfNeeded();

  // If we changed contexts, then we always want to restore it, since the caller
  // doesn't know that we're switching contexts.
  if (scoped_make_current)
    mode = kDoRestoreBindings;

  // Save the current binding if requested.
  GLint bound_service_id = 0;
  if (mode == kDoRestoreBindings)
    glGetIntegerv(GL_TEXTURE_BINDING_EXTERNAL_OES, &bound_service_id);

  shared_state_->UpdateTexImage();
  if (mode == kDoRestoreBindings)
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, bound_service_id);
}

void AVDACodecImage::UpdateSurface(UpdateMode update_mode) {
  UpdateSurfaceInternal(update_mode, kDoRestoreBindings);
}

void AVDACodecImage::CodecChanged(MediaCodecBridge* codec) {
  media_codec_ = codec;
  codec_buffer_index_ = kInvalidCodecBufferIndex;
}

void AVDACodecImage::SetBufferMetadata(int buffer_index,
                                       bool has_texture_owner,
                                       const gfx::Size& size) {
  has_texture_owner_ = has_texture_owner;
  codec_buffer_index_ = buffer_index;
  size_ = size;
}

bool AVDACodecImage::SetSharedState(
    scoped_refptr<AVDASharedState> shared_state) {
  if (shared_state == shared_state_)
    return false;
  shared_state_ = shared_state;
  most_recent_bounds_ = gfx::Rect();
  return true;
}

void AVDACodecImage::UpdateSurfaceInternal(
    UpdateMode update_mode,
    RestoreBindingsMode attached_bindings_mode) {
  if (!IsCodecBufferOutstanding())
    return;

  ReleaseOutputBuffer(update_mode);

  // SurfaceViews are updated implicitly, so no further steps are necessary.
  if (!has_texture_owner_) {
    DCHECK(update_mode != UpdateMode::RENDER_TO_BACK_BUFFER);
    return;
  }

  // If front buffer rendering hasn't been requested, exit early.
  if (update_mode != UpdateMode::RENDER_TO_FRONT_BUFFER)
    return;

  UpdateTextureOwner(attached_bindings_mode);
}

void AVDACodecImage::ReleaseOutputBuffer(UpdateMode update_mode) {
  DCHECK(IsCodecBufferOutstanding());

  // In case of discard, simply discard and clear our codec buffer index.
  if (update_mode == UpdateMode::DISCARD_CODEC_BUFFER) {
    if (codec_buffer_index_ != kUpdateOnly)
      media_codec_->ReleaseOutputBuffer(codec_buffer_index_, false);

    // Note: No need to wait for the frame to be available in the kUpdateOnly
    // case since it will be or has been waited on by another release call.
    codec_buffer_index_ = kInvalidCodecBufferIndex;
    return;
  }

  DCHECK(update_mode == UpdateMode::RENDER_TO_BACK_BUFFER ||
         update_mode == UpdateMode::RENDER_TO_FRONT_BUFFER);

  if (!has_texture_owner_) {
    DCHECK(update_mode == UpdateMode::RENDER_TO_FRONT_BUFFER);
    DCHECK_GE(codec_buffer_index_, 0);
    media_codec_->ReleaseOutputBuffer(codec_buffer_index_, true);
    codec_buffer_index_ = kRendered;
    return;
  }

  // If we've already released to the back buffer, there's nothing left to do,
  // but wait for the previously released buffer if necessary.
  if (codec_buffer_index_ != kUpdateOnly) {
    DCHECK(has_texture_owner_);
    DCHECK_GE(codec_buffer_index_, 0);
    shared_state_->RenderCodecBufferToTextureOwner(media_codec_,
                                                   codec_buffer_index_);
    codec_buffer_index_ = kUpdateOnly;
  }

  // Only wait for the TextureOwner update if we're rendering to the front.
  if (update_mode == UpdateMode::RENDER_TO_FRONT_BUFFER)
    shared_state_->WaitForFrameAvailable();
}

std::unique_ptr<ui::ScopedMakeCurrent> AVDACodecImage::MakeCurrentIfNeeded() {
  DCHECK(shared_state_->context());
  // Remember: virtual contexts return true if and only if their shared context
  // is current, regardless of which virtual context it is.
  return std::unique_ptr<ui::ScopedMakeCurrent>(
      shared_state_->context()->IsCurrent(nullptr)
          ? nullptr
          : new ui::ScopedMakeCurrent(shared_state_->context(),
                                      shared_state_->surface()));
}

void AVDACodecImage::GetTextureMatrix(float matrix[16]) {
  // Our current matrix may be stale.  Update it if possible.
  if (has_texture_owner_)
    UpdateSurface(UpdateMode::RENDER_TO_FRONT_BUFFER);
  shared_state_->GetTransformMatrix(matrix);
  YInvertMatrix(matrix);
}

void AVDACodecImage::NotifyPromotionHint(bool promotion_hint,
                                         int display_x,
                                         int display_y,
                                         int display_width,
                                         int display_height) {
  shared_state_->GetPromotionHintCB().Run(PromotionHintAggregator::Hint(
      gfx::Rect(display_x, display_y, display_width, display_height),
      promotion_hint));
}

bool AVDACodecImage::IsCodecBufferOutstanding() const {
  static_assert(kUpdateOnly < 0 && kUpdateOnly > kRendered &&
                    kRendered > kInvalidCodecBufferIndex,
                "Codec buffer index enum values are not ordered correctly.");
  return codec_buffer_index_ > kRendered && media_codec_;
}

}  // namespace media
