// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/gpu/vaapi/vaapi_picture_factory.h"

#include "media/gpu/vaapi/vaapi_wrapper.h"
#include "ui/gl/gl_bindings.h"

#if defined(USE_X11)
#include "media/gpu/vaapi/vaapi_picture_tfp.h"
#endif
#if defined(USE_OZONE)
#include "media/gpu/vaapi/vaapi_picture_native_pixmap_ozone.h"
#endif
#if defined(USE_EGL)
#include "media/gpu/vaapi/vaapi_picture_native_pixmap_egl.h"
#endif

namespace media {

namespace {

const struct {
  gl::GLImplementation gl_impl;
  VaapiPictureFactory::VaapiImplementation va_impl;
} kVaapiImplementationPairs[] = {{gl::kGLImplementationEGLGLES2,
                                  VaapiPictureFactory::kVaapiImplementationDrm}
#if defined(USE_X11)
                                 ,
                                 {gl::kGLImplementationDesktopGL,
                                  VaapiPictureFactory::kVaapiImplementationX11}
#endif  // USE_X11
};

}  // namespace

VaapiPictureFactory::VaapiPictureFactory() = default;

VaapiPictureFactory::~VaapiPictureFactory() = default;

std::unique_ptr<VaapiPicture> VaapiPictureFactory::Create(
    const scoped_refptr<VaapiWrapper>& vaapi_wrapper,
    const MakeGLContextCurrentCallback& make_context_current_cb,
    const BindGLImageCallback& bind_image_cb,
    int32_t picture_buffer_id,
    const gfx::Size& size,
    uint32_t texture_id,
    uint32_t client_texture_id,
    uint32_t texture_target) {
  DCHECK_EQ(texture_target, GetGLTextureTarget());

  std::unique_ptr<VaapiPicture> picture;

  // Select DRM(egl) / TFP(glx) at runtime with --use-gl=egl / --use-gl=desktop
  switch (GetVaapiImplementation(gl::GetGLImplementation())) {
#if defined(USE_OZONE)
    // We can be called without GL initialized, which is valid if we use Ozone.
    case kVaapiImplementationNone:
      FALLTHROUGH;
    case kVaapiImplementationDrm:
      picture.reset(new VaapiPictureNativePixmapOzone(
          vaapi_wrapper, make_context_current_cb, bind_image_cb,
          picture_buffer_id, size, texture_id, client_texture_id,
          texture_target));
      break;
#elif defined(USE_EGL)
    case kVaapiImplementationDrm:
      picture.reset(new VaapiPictureNativePixmapEgl(
          vaapi_wrapper, make_context_current_cb, bind_image_cb,
          picture_buffer_id, size, texture_id, client_texture_id,
          texture_target));
      break;
#endif

#if defined(USE_X11)
    case kVaapiImplementationX11:
      picture.reset(new VaapiTFPPicture(vaapi_wrapper, make_context_current_cb,
                                        bind_image_cb, picture_buffer_id, size,
                                        texture_id, client_texture_id,
                                        texture_target));

      break;
#endif  // USE_X11

    default:
      NOTREACHED();
      return nullptr;
  }

  return picture;
}

VaapiPictureFactory::VaapiImplementation
VaapiPictureFactory::GetVaapiImplementation(gl::GLImplementation gl_impl) {
  for (const auto& implementation_pair : kVaapiImplementationPairs) {
    if (gl_impl == implementation_pair.gl_impl)
      return implementation_pair.va_impl;
  }

  return kVaapiImplementationNone;
}

uint32_t VaapiPictureFactory::GetGLTextureTarget() {
#if defined(USE_OZONE)
  return GL_TEXTURE_EXTERNAL_OES;
#else
  return GL_TEXTURE_2D;
#endif
}

gfx::BufferFormat VaapiPictureFactory::GetBufferFormat() {
#if defined(USE_OZONE)
  return gfx::BufferFormat::YUV_420_BIPLANAR;
#else
  return gfx::BufferFormat::RGBX_8888;
#endif
}

}  // namespace media
