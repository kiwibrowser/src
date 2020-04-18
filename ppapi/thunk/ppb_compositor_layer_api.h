// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_THUNK_PPB_COMPOSITOR_LAYER_API_H_
#define PPAPI_THUNK_PPB_COMPOSITOR_LAYER_API_H_

#include <stdint.h>

#include "base/memory/ref_counted.h"
#include "ppapi/c/ppb_compositor_layer.h"
#include "ppapi/shared_impl/tracked_callback.h"

namespace ppapi {
namespace thunk {

class PPAPI_THUNK_EXPORT PPB_CompositorLayer_API {
 public:
  virtual ~PPB_CompositorLayer_API() {}
  virtual int32_t SetColor(float red,
                           float green,
                           float blue,
                           float alpha,
                           const PP_Size* size) = 0;
  virtual int32_t SetTexture0_1(
      PP_Resource context,
      uint32_t texture,
      const PP_Size* size,
      const scoped_refptr<ppapi::TrackedCallback>& callback) = 0;
  virtual int32_t SetTexture(
      PP_Resource context,
      uint32_t target,
      uint32_t texture,
      const PP_Size* size,
      const scoped_refptr<ppapi::TrackedCallback>& callback) = 0;
  virtual int32_t SetImage(
      PP_Resource image_data,
      const PP_Size* size,
      const scoped_refptr<ppapi::TrackedCallback>& callback) = 0;
  virtual int32_t SetClipRect(const PP_Rect* rects) = 0;
  virtual int32_t SetTransform(const float matrix[16]) = 0;
  virtual int32_t SetOpacity(float opacity) = 0;
  virtual int32_t SetBlendMode(PP_BlendMode mode) = 0;
  virtual int32_t SetSourceRect(const PP_FloatRect* rect) = 0;
  virtual int32_t SetPremultipliedAlpha(PP_Bool premult) = 0;
};

}  // namespace thunk
}  // namespace ppapi

#endif  // PPAPI_THUNK_PPB_COMPOSITOR_API_H_
