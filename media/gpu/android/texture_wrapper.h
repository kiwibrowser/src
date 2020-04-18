// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_GPU_ANDROID_TEXTURE_WRAPPER_H_
#define MEDIA_GPU_ANDROID_TEXTURE_WRAPPER_H_

#include "base/memory/scoped_refptr.h"

namespace gpu {
namespace gles2 {
class TextureRef;
}  // namespace gles2
}  // namespace gpu

namespace media {

// Temporary class to allow mocking a TextureRef, which has no virtual methods.
// It is expected that this will be replaced by gpu::gles2::AbstractTexture in
// the near future, will will support mocks directly.
class TextureWrapper {
 public:
  virtual ~TextureWrapper() = default;
  virtual void ForceContextLost() = 0;
};

// Since these are temporary classes, the impl might as well go in the same
// file for easier cleanup later.
class TextureWrapperImpl : public TextureWrapper {
 public:
  TextureWrapperImpl(scoped_refptr<gpu::gles2::TextureRef> texture_ref);
  ~TextureWrapperImpl() override;

  // TextureWrapper
  void ForceContextLost() override;

 private:
  scoped_refptr<gpu::gles2::TextureRef> texture_ref_;
};

}  // namespace media

#endif  // MEDIA_GPU_ANDROID_TEXTURE_WRAPPER_H_
