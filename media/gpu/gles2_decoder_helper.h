// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_GPU_GLES2_DECODER_HELPER_H_
#define MEDIA_GPU_GLES2_DECODER_HELPER_H_

#include <stdint.h>

#include <memory>

#include "base/memory/ref_counted.h"
#include "base/optional.h"
#include "media/gpu/media_gpu_export.h"
#include "ui/gl/gl_bindings.h"

namespace gpu {
class DecoderContext;
struct Mailbox;
namespace gles2 {
class TextureRef;
}  // namespace gles2
}  // namespace gpu

namespace gl {
class GLContext;
class GLImage;
}  // namespace gl

namespace media {

// Utility methods to simplify working with a gpu::DecoderContext from
// inside VDAs.
class MEDIA_GPU_EXPORT GLES2DecoderHelper {
 public:
  static std::unique_ptr<GLES2DecoderHelper> Create(
      gpu::DecoderContext* decoder);

  virtual ~GLES2DecoderHelper() {}

  // TODO(sandersd): Provide scoped version?
  virtual bool MakeContextCurrent() = 0;

  // Creates a texture and configures it as a video frame (linear filtering,
  // clamp to edge). The context must be current.
  //
  // See glTexImage2D() for parameter definitions.
  //
  // Returns nullptr on failure, but there are currently no failure paths.
  virtual scoped_refptr<gpu::gles2::TextureRef> CreateTexture(
      GLenum target,
      GLenum internal_format,
      GLsizei width,
      GLsizei height,
      GLenum format,
      GLenum type) = 0;

  // Sets the cleared flag on level 0 of the texture.
  virtual void SetCleared(gpu::gles2::TextureRef* texture_ref) = 0;

  // Binds level 0 of the texture to an image.
  virtual void BindImage(gpu::gles2::TextureRef* texture_ref,
                         gl::GLImage* image,
                         bool can_bind_to_sampler) = 0;

  // Gets the associated GLContext.
  virtual gl::GLContext* GetGLContext() = 0;

  // Creates a mailbox for a texture.
  virtual gpu::Mailbox CreateMailbox(gpu::gles2::TextureRef* texture_ref) = 0;
};

}  // namespace media

#endif  // MEDIA_GPU_GLES2_DECODER_HELPER_H_
