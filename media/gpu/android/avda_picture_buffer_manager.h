// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_GPU_ANDROID_AVDA_PICTURE_BUFFER_MANAGER_H_
#define MEDIA_GPU_ANDROID_AVDA_PICTURE_BUFFER_MANAGER_H_

#include <stdint.h>
#include <vector>

#include "base/macros.h"
#include "media/gpu/android/avda_state_provider.h"
#include "media/gpu/android/avda_surface_bundle.h"
#include "media/gpu/android/surface_texture_gl_owner.h"
#include "media/gpu/media_gpu_export.h"

namespace gpu {
namespace gles2 {
class GLStreamTextureImage;
}
}  // namespace gpu

namespace media {
class AVDACodecImage;
class AVDASharedState;
class MediaCodecBridge;

// AVDAPictureBufferManager is used by AVDA to associate its PictureBuffers with
// MediaCodec output buffers. It attaches AVDACodecImages to the PictureBuffer
// textures so that when they're used to draw the AVDACodecImage can release the
// MediaCodec buffer to the backing Surface. If the Surface is a TextureOwner,
// the front buffer can then be used to draw without needing to copy the pixels.
// If the Surface is a SurfaceView, the release causes the frame to be displayed
// immediately.
class MEDIA_GPU_EXPORT AVDAPictureBufferManager {
 public:
  using PictureBufferMap = std::map<int32_t, PictureBuffer>;

  explicit AVDAPictureBufferManager(AVDAStateProvider* state_provider);
  virtual ~AVDAPictureBufferManager();

  // Call Initialize, providing the surface bundle that holds the surface that
  // will back the frames.  If an overlay is present in the bundle, then this
  // will set us up to render codec buffers at the appropriate time for display,
  // but will assume that consuming the resulting buffers is handled elsewhere
  // (e.g., SurfaceFlinger).  We will ensure that any reference to the bundle
  // is dropped if the overlay sends OnSurfaceDestroyed.
  //
  // Without an overlay, we will create a TextureOwner and add it (and its
  // surface) to |surface_bundle|.  We will arrange to consume the buffers at
  // the right time, in addition to releasing the codec buffers for rendering.
  //
  // One may call these multiple times to change between overlay and ST.
  //
  // Picture buffers will be updated to reflect the new surface during the call
  // to UseCodecBufferForPicture().
  //
  // Returns true on success.
  bool Initialize(scoped_refptr<AVDASurfaceBundle> surface_bundle);

  void Destroy(const PictureBufferMap& buffers);

  // Sets up |picture_buffer| so that its texture will refer to the image that
  // is represented by the decoded output buffer at codec_buffer_index.
  void UseCodecBufferForPictureBuffer(int32_t codec_buffer_index,
                                      const PictureBuffer& picture_buffer);

  // Assigns a picture buffer and attaches an image to its texture.
  void AssignOnePictureBuffer(const PictureBuffer& picture_buffer,
                              bool have_context);

  // Reuses a picture buffer to hold a new frame.
  void ReuseOnePictureBuffer(const PictureBuffer& picture_buffer);

  // Release MediaCodec buffers.
  void ReleaseCodecBuffers(const PictureBufferMap& buffers);

  // Attempts to free up codec output buffers by rendering early.
  void MaybeRenderEarly();

  // Called when the MediaCodec instance changes. If |codec| is nullptr the
  // MediaCodec is being destroyed. Previously provided codecs should no longer
  // be referenced.
  void CodecChanged(MediaCodecBridge* codec);

  // Whether the pictures buffers are overlayable.
  bool ArePicturesOverlayable();

  // Are there any unrendered picture buffers oustanding?
  bool HasUnrenderedPictures() const;

  // Returns the GL texture target that the PictureBuffer textures use.
  // Always use OES textures even though this will cause flickering in dev tools
  // when inspecting a fullscreen video.  See http://crbug.com/592798
  static constexpr GLenum kTextureTarget = GL_TEXTURE_EXTERNAL_OES;

 private:
  // Release any codec buffer that is associated with the given picture buffer
  // back to the codec.  It is okay if there is no such buffer.
  void ReleaseCodecBufferForPicture(const PictureBuffer& picture_buffer);

  // Sets up the texture references (as found by |picture_buffer|), for the
  // specified |image|. If |image| is null, clears any ref on the texture
  // associated with |picture_buffer|.
  void SetImageForPicture(const PictureBuffer& picture_buffer,
                          gpu::gles2::GLStreamTextureImage* image);

  AVDACodecImage* GetImageForPicture(int picture_buffer_id) const;

  scoped_refptr<AVDASharedState> shared_state_;

  AVDAStateProvider* const state_provider_;

  // The texture owner to render to. Non-null after Initialize() if
  // we're not rendering to a SurfaceView.
  scoped_refptr<TextureOwner> texture_owner_;

  MediaCodecBridge* media_codec_;

  // Picture buffer IDs that are out for display. Stored in order of frames as
  // they are returned from the decoder.
  std::vector<int32_t> pictures_out_for_display_;

  // Maps a picture buffer id to a AVDACodecImage.
  std::map<int, scoped_refptr<AVDACodecImage>> codec_images_;

  DISALLOW_COPY_AND_ASSIGN(AVDAPictureBufferManager);
};

}  // namespace media

#endif  // MEDIA_GPU_ANDROID_AVDA_PICTURE_BUFFER_MANAGER_H_
