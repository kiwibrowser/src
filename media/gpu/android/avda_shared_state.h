// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_GPU_ANDROID_AVDA_SHARED_STATE_H_
#define MEDIA_GPU_ANDROID_AVDA_SHARED_STATE_H_

#include "base/memory/weak_ptr.h"
#include "base/synchronization/waitable_event.h"
#include "gpu/command_buffer/service/gles2_cmd_decoder.h"
#include "media/base/android/android_overlay.h"
#include "media/base/android/media_codec_bridge.h"
#include "media/gpu/android/avda_shared_state.h"
#include "media/gpu/android/avda_surface_bundle.h"
#include "media/gpu/android/promotion_hint_aggregator.h"
#include "ui/gl/gl_bindings.h"
#include "ui/gl/gl_context.h"
#include "ui/gl/gl_image.h"
#include "ui/gl/gl_surface.h"

namespace media {

// State shared by AVDACodecImages.  This holds a reference to the surface
// bundle that's backing the frames.  If it's an overlay, then we'll
// automatically drop our reference to the bundle if the overlay's surface gets
// an OnSurfaceDestroyed.
// TODO(watk): This doesn't really do anything any more; we should delete it.
class AVDASharedState : public base::RefCounted<AVDASharedState> {
 public:
  AVDASharedState(scoped_refptr<AVDASurfaceBundle> surface_bundle);

  GLuint texture_owner_service_id() const {
    return texture_owner() ? texture_owner()->GetTextureId() : 0;
  }

  TextureOwner* texture_owner() const {
    return surface_bundle_ ? surface_bundle_->texture_owner_.get() : nullptr;
  }

  AndroidOverlay* overlay() const {
    return surface_bundle_ ? surface_bundle_->overlay.get() : nullptr;
  }

  // Context and surface that |texture_owner_| is bound to, if
  // |texture_owner_| is not null.
  gl::GLContext* context() const {
    return texture_owner() ? texture_owner()->GetContext() : nullptr;
  }

  gl::GLSurface* surface() const {
    return texture_owner() ? texture_owner()->GetSurface() : nullptr;
  }

  // Helper method for coordinating the interactions between
  // MediaCodec::ReleaseOutputBuffer() and WaitForFrameAvailable() when
  // rendering to a TextureOwner; this method should never be called when
  // rendering to a SurfaceView.
  //
  // The release of the codec buffer to the texture owner is asynchronous, by
  // using this helper we can attempt to let this process complete in a non
  // blocking fashion before the TextureOwner is used.
  //
  // Clients should call this method to release the codec buffer for rendering
  // and then call WaitForFrameAvailable() before using the TextureOwner. In
  // the ideal case the TextureOwner has already been updated, otherwise the
  // method will wait for a pro-rated amount of time based on elapsed time up
  // to a short deadline.
  //
  // Some devices do not reliably notify frame availability, so we use a very
  // short deadline of only a few milliseconds to avoid indefinite stalls.
  void RenderCodecBufferToTextureOwner(MediaCodecBridge* codec,
                                       int codec_buffer_index);

  void WaitForFrameAvailable();

  // Helper methods for interacting with |texture_owner_|. See
  // gl::TextureOwner for method details.
  void UpdateTexImage();

  // Returns a matrix that needs to be y flipped in order to match the
  // StreamTextureMatrix contract. See GLStreamTextureImage::YInvertMatrix().
  void GetTransformMatrix(float matrix[16]) const;

  // Resets the last time for RenderCodecBufferToTextureOwner(). Should be
  // called during codec changes.
  void ClearReleaseTime();

  void ClearOverlay(AndroidOverlay* overlay);

  void SetPromotionHintCB(PromotionHintAggregator::NotifyPromotionHintCB cb);
  const PromotionHintAggregator::NotifyPromotionHintCB& GetPromotionHintCB();

 protected:
  virtual ~AVDASharedState();

 private:
  friend class base::RefCounted<AVDASharedState>;

  // Texture matrix of the front buffer of the texture owner.
  float gl_matrix_[16];

  scoped_refptr<AVDASurfaceBundle> surface_bundle_;

  PromotionHintAggregator::NotifyPromotionHintCB promotion_hint_cb_;

  base::WeakPtrFactory<AVDASharedState> weak_this_factory_;

  DISALLOW_COPY_AND_ASSIGN(AVDASharedState);
};

}  // namespace media

#endif  // MEDIA_GPU_ANDROID_AVDA_SHARED_STATE_H_
