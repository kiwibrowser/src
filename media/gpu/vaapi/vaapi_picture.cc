// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/gpu/vaapi/vaapi_picture.h"

#include "media/gpu/vaapi/vaapi_wrapper.h"
#include "ui/gl/gl_bindings.h"
#include "ui/gl/gl_implementation.h"

namespace media {

VaapiPicture::VaapiPicture(
    const scoped_refptr<VaapiWrapper>& vaapi_wrapper,
    const MakeGLContextCurrentCallback& make_context_current_cb,
    const BindGLImageCallback& bind_image_cb,
    int32_t picture_buffer_id,
    const gfx::Size& size,
    uint32_t texture_id,
    uint32_t client_texture_id,
    uint32_t texture_target)
    : vaapi_wrapper_(vaapi_wrapper),
      make_context_current_cb_(make_context_current_cb),
      bind_image_cb_(bind_image_cb),
      size_(size),
      texture_id_(texture_id),
      client_texture_id_(client_texture_id),
      texture_target_(texture_target),
      picture_buffer_id_(picture_buffer_id) {}

VaapiPicture::~VaapiPicture() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

bool VaapiPicture::AllowOverlay() const {
  return false;
}

}  // namespace media
