// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/gpu/android/texture_wrapper.h"

#include "gpu/command_buffer/service/texture_manager.h"

namespace media {

TextureWrapperImpl::TextureWrapperImpl(
    scoped_refptr<gpu::gles2::TextureRef> texture_ref)
    : texture_ref_(std::move(texture_ref)) {}

TextureWrapperImpl::~TextureWrapperImpl() {}

void TextureWrapperImpl::ForceContextLost() {
  if (texture_ref_)
    texture_ref_->ForceContextLost();
}

}  // namespace media
