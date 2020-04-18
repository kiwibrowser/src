// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_SHARED_IMPL_COMPOSITOR_LAYER_DATA_H_
#define PPAPI_SHARED_IMPL_COMPOSITOR_LAYER_DATA_H_

#include <stdint.h>
#include <string.h>

#include <memory>

#include "base/logging.h"
#include "gpu/command_buffer/common/mailbox.h"
#include "gpu/command_buffer/common/sync_token.h"
#include "ppapi/c/ppb_compositor_layer.h"
#include "ppapi/shared_impl/host_resource.h"
#include "ppapi/shared_impl/ppapi_shared_export.h"

namespace ppapi {

struct PPAPI_SHARED_EXPORT CompositorLayerData {

  struct Transform {
    Transform() {
      matrix[0] = 1.0f;
      matrix[1] = 0.0f;
      matrix[2] = 0.0f;
      matrix[3] = 0.0f;
      matrix[4] = 0.0f;
      matrix[5] = 1.0f;
      matrix[6] = 0.0f;
      matrix[7] = 0.0f;
      matrix[8] = 0.0f;
      matrix[9] = 0.0f;
      matrix[10] = 1.0f;
      matrix[11] = 0.0f;
      matrix[12] = 0.0f;
      matrix[13] = 0.0f;
      matrix[14] = 0.0f;
      matrix[15] = 1.0f;
    }

    float matrix[16];
  };

  struct LayerCommon {
    LayerCommon()
       : size(PP_MakeSize(0, 0)),
         clip_rect(PP_MakeRectFromXYWH(0, 0, 0, 0)),
         blend_mode(PP_BLENDMODE_SRC_OVER),
         opacity(1.0f),
         resource_id(0) {
    }

    PP_Size size;
    PP_Rect clip_rect;
    Transform transform;
    PP_BlendMode blend_mode;
    float opacity;
    uint32_t resource_id;
  };

  struct ColorLayer {
    ColorLayer() : red(0.0f), green(0.0f), blue(0.0f), alpha(0.0f) {}

    float red;
    float green;
    float blue;
    float alpha;
  };

  struct ImageLayer {
    ImageLayer()
       : resource(0),
         source_rect(PP_MakeFloatRectFromXYWH(0.0f, 0.0f, 0.0f, 0.0f)) {}

    PP_Resource resource;
    PP_FloatRect source_rect;
  };

  struct TextureLayer {
    TextureLayer()
       : target(0),
         source_rect(PP_MakeFloatRectFromXYWH(0.0f, 0.0f, 1.0f, 1.0f)),
         premult_alpha(true) {}

    gpu::Mailbox mailbox;
    gpu::SyncToken sync_token;
    uint32_t target;
    PP_FloatRect source_rect;
    bool premult_alpha;
  };

  CompositorLayerData() {}

  CompositorLayerData(const CompositorLayerData& other) {
    *this = other;
  }

  bool is_null() const {
    return !(color || texture || image);
  }

  bool is_valid() const {
    int i = 0;
    if (color) ++i;
    if (texture) ++i;
    if (image) ++i;
    return i == 1;
  }

  const CompositorLayerData& operator=(const CompositorLayerData& other);

  LayerCommon common;
  std::unique_ptr<ColorLayer> color;
  std::unique_ptr<TextureLayer> texture;
  std::unique_ptr<ImageLayer> image;
};

}  // namespace ppapi

#endif  // PPAPI_SHARED_IMPL_COMPOSITOR_LAYER_DATA_H_
