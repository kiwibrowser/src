// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/gpu/android/avda_picture_buffer_manager.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "base/android/build_info.h"
#include "base/bind.h"
#include "base/logging.h"
#include "base/metrics/histogram.h"
#include "gpu/command_buffer/service/context_group.h"
#include "gpu/command_buffer/service/gl_stream_texture_image.h"
#include "gpu/command_buffer/service/gles2_cmd_copy_texture_chromium.h"
#include "gpu/command_buffer/service/texture_manager.h"
#include "gpu/ipc/service/gpu_channel.h"
#include "media/base/android/media_codec_bridge_impl.h"
#include "media/gpu/android/avda_codec_image.h"
#include "media/gpu/android/avda_shared_state.h"
#include "ui/gl/android/scoped_java_surface.h"
#include "ui/gl/android/surface_texture.h"
#include "ui/gl/egl_util.h"
#include "ui/gl/gl_bindings.h"
#include "ui/gl/gl_surface_egl.h"
#include "ui/gl/scoped_binders.h"
#include "ui/gl/scoped_make_current.h"

// If !|ptr|, log a message, notify |state_provider_| of the error, and
// return an optional value.
#define RETURN_IF_NULL(ptr, ...)                                           \
  do {                                                                     \
    if (!(ptr)) {                                                          \
      DLOG(ERROR) << "Got null for " << #ptr;                              \
      state_provider_->NotifyError(VideoDecodeAccelerator::ILLEGAL_STATE); \
      return __VA_ARGS__;                                                  \
    }                                                                      \
  } while (0)

namespace media {

AVDAPictureBufferManager::AVDAPictureBufferManager(
    AVDAStateProvider* state_provider)
    : state_provider_(state_provider), media_codec_(nullptr) {}

AVDAPictureBufferManager::~AVDAPictureBufferManager() {}

bool AVDAPictureBufferManager::Initialize(
    scoped_refptr<AVDASurfaceBundle> surface_bundle) {
  shared_state_ = nullptr;
  texture_owner_ = nullptr;

  if (!surface_bundle->overlay) {
    // Create the texture owner.
    texture_owner_ = SurfaceTextureGLOwner::Create();
    if (!texture_owner_)
      return false;

    surface_bundle->texture_owner_surface = texture_owner_->CreateJavaSurface();
    surface_bundle->texture_owner_ = texture_owner_;
  }

  // Only do this once the texture owner is filled in, since the constructor
  // assumes that it will be.
  shared_state_ = new AVDASharedState(surface_bundle);
  shared_state_->SetPromotionHintCB(state_provider_->GetPromotionHintCB());

  return true;
}

void AVDAPictureBufferManager::Destroy(const PictureBufferMap& buffers) {
  // Do nothing if Initialize() has not been called.
  if (!shared_state_)
    return;

  ReleaseCodecBuffers(buffers);
  CodecChanged(nullptr);
  texture_owner_ = nullptr;
}

void AVDAPictureBufferManager::SetImageForPicture(
    const PictureBuffer& picture_buffer,
    gpu::gles2::GLStreamTextureImage* image) {
  auto* context_group = state_provider_->GetContextGroup();
  RETURN_IF_NULL(context_group);
  auto* texture_manager = context_group->texture_manager();
  RETURN_IF_NULL(texture_manager);

  DCHECK_LE(1u, picture_buffer.client_texture_ids().size());
  gpu::gles2::TextureRef* texture_ref =
      texture_manager->GetTexture(picture_buffer.client_texture_ids()[0]);
  RETURN_IF_NULL(texture_ref);

  // Default to zero which will clear the stream texture service id if one was
  // previously set.
  GLuint stream_texture_service_id = 0;
  if (image) {
    // Override the Texture's service id, so that it will use the one that is
    // attached to the TextureOwner
    stream_texture_service_id = shared_state_->texture_owner_service_id();

    // Also set the parameters for the level if we're not clearing the image.
    const gfx::Size size = state_provider_->GetSize();
    texture_manager->SetLevelInfo(texture_ref, kTextureTarget, 0, GL_RGBA,
                                  size.width(), size.height(), 1, 0, GL_RGBA,
                                  GL_UNSIGNED_BYTE, gfx::Rect());

    static_cast<AVDACodecImage*>(image)->set_texture(texture_ref->texture());
  }

  // If we're clearing the image, or setting a TextureOwner backed image, we
  // set the state to UNBOUND. For TextureOwner images, this ensures that the
  // implementation will call CopyTexImage, which is where AVDACodecImage
  // updates the TextureOwner to the right frame.
  auto image_state = gpu::gles2::Texture::UNBOUND;
  // For SurfaceView we set the state to BOUND because ScheduleOverlayPlane
  // requires it. If something tries to sample from this texture it won't work,
  // but there's no way to sample from a SurfaceView anyway, so it doesn't
  // matter.
  if (image && !texture_owner_)
    image_state = gpu::gles2::Texture::BOUND;
  texture_manager->SetLevelStreamTextureImage(texture_ref, kTextureTarget, 0,
                                              image, image_state,
                                              stream_texture_service_id);
  texture_manager->SetLevelCleared(texture_ref, kTextureTarget, 0, true);
}

AVDACodecImage* AVDAPictureBufferManager::GetImageForPicture(
    int picture_buffer_id) const {
  auto it = codec_images_.find(picture_buffer_id);
  DCHECK(it != codec_images_.end());
  return it->second.get();
}

void AVDAPictureBufferManager::UseCodecBufferForPictureBuffer(
    int32_t codec_buf_index,
    const PictureBuffer& picture_buffer) {
  // Notify the AVDACodecImage for picture_buffer that it should use the
  // decoded buffer codec_buf_index to render this frame.
  AVDACodecImage* avda_image = GetImageForPicture(picture_buffer.id());

  // Note that this is not a race, since we do not re-use a PictureBuffer
  // until after the CC is done drawing it.
  pictures_out_for_display_.push_back(picture_buffer.id());
  avda_image->SetBufferMetadata(codec_buf_index, !!texture_owner_,
                                state_provider_->GetSize());

  // If the shared state has changed for this image, retarget its texture.
  if (avda_image->SetSharedState(shared_state_))
    SetImageForPicture(picture_buffer, avda_image);

  MaybeRenderEarly();
}

void AVDAPictureBufferManager::AssignOnePictureBuffer(
    const PictureBuffer& picture_buffer,
    bool have_context) {
  // Attach a GLImage to each texture that will use the texture owner.
  scoped_refptr<gpu::gles2::GLStreamTextureImage> gl_image =
      codec_images_[picture_buffer.id()] =
          new AVDACodecImage(shared_state_, media_codec_);
  SetImageForPicture(picture_buffer, gl_image.get());
}

void AVDAPictureBufferManager::ReleaseCodecBufferForPicture(
    const PictureBuffer& picture_buffer) {
  GetImageForPicture(picture_buffer.id())
      ->UpdateSurface(AVDACodecImage::UpdateMode::DISCARD_CODEC_BUFFER);
}

void AVDAPictureBufferManager::ReuseOnePictureBuffer(
    const PictureBuffer& picture_buffer) {
  pictures_out_for_display_.erase(
      std::remove(pictures_out_for_display_.begin(),
                  pictures_out_for_display_.end(), picture_buffer.id()),
      pictures_out_for_display_.end());

  // At this point, the CC must be done with the picture.  We can't really
  // check for that here directly.  it's guaranteed in gpu_video_decoder.cc,
  // when it waits on the sync point before releasing the mailbox.  That sync
  // point is inserted by destroying the resource in VideoLayerImpl::DidDraw.
  ReleaseCodecBufferForPicture(picture_buffer);
  MaybeRenderEarly();
}

void AVDAPictureBufferManager::ReleaseCodecBuffers(
    const PictureBufferMap& buffers) {
  for (const std::pair<int, PictureBuffer>& entry : buffers)
    ReleaseCodecBufferForPicture(entry.second);
}

void AVDAPictureBufferManager::MaybeRenderEarly() {
  if (pictures_out_for_display_.empty())
    return;

  // See if we can consume the front buffer / render to the SurfaceView. Iterate
  // in reverse to find the most recent front buffer. If none is found, the
  // |front_index| will point to the beginning of the array.
  size_t front_index = pictures_out_for_display_.size() - 1;
  AVDACodecImage* first_renderable_image = nullptr;
  for (int i = front_index; i >= 0; --i) {
    const int id = pictures_out_for_display_[i];
    AVDACodecImage* avda_image = GetImageForPicture(id);

    // Update the front buffer index as we move along to shorten the number of
    // candidate images we look at for back buffer rendering.
    front_index = i;
    first_renderable_image = avda_image;

    // If we find a front buffer, stop and indicate that front buffer rendering
    // is not possible since another image is already in the front buffer.
    if (avda_image->was_rendered_to_front_buffer()) {
      first_renderable_image = nullptr;
      break;
    }
  }

  if (first_renderable_image) {
    first_renderable_image->UpdateSurface(
        AVDACodecImage::UpdateMode::RENDER_TO_FRONT_BUFFER);
  }

  // Back buffer rendering is only available for texture owners. We'll always
  // have at least one front buffer, so the next buffer must be the backbuffer.
  size_t backbuffer_index = front_index + 1;
  if (!texture_owner_ || backbuffer_index >= pictures_out_for_display_.size())
    return;

  // See if the back buffer is free. If so, then render the frame adjacent to
  // the front buffer.  The listing is in render order, so we can just use the
  // first unrendered frame if there is back buffer space.
  first_renderable_image =
      GetImageForPicture(pictures_out_for_display_[backbuffer_index]);
  if (first_renderable_image->was_rendered_to_back_buffer())
    return;

  // Due to the loop in the beginning this should never be true.
  DCHECK(!first_renderable_image->was_rendered_to_front_buffer());
  first_renderable_image->UpdateSurface(
      AVDACodecImage::UpdateMode::RENDER_TO_BACK_BUFFER);
}

void AVDAPictureBufferManager::CodecChanged(MediaCodecBridge* codec) {
  media_codec_ = codec;
  for (auto& image_kv : codec_images_)
    image_kv.second->CodecChanged(codec);
  shared_state_->ClearReleaseTime();
}

bool AVDAPictureBufferManager::ArePicturesOverlayable() {
  // SurfaceView frames are always overlayable because that's the only way to
  // display them.
  return !texture_owner_;
}

bool AVDAPictureBufferManager::HasUnrenderedPictures() const {
  for (int id : pictures_out_for_display_) {
    if (GetImageForPicture(id)->is_unrendered())
      return true;
  }
  return false;
}

}  // namespace media
