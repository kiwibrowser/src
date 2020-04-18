// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_SERVICE_DISPLAY_COLOR_LUT_CACHE_H_
#define COMPONENTS_VIZ_SERVICE_DISPLAY_COLOR_LUT_CACHE_H_

#include <map>

#include "base/containers/mru_cache.h"
#include "base/macros.h"
#include "components/viz/service/viz_service_export.h"
#include "ui/gfx/color_space.h"

namespace gfx {
class ColorTransform;
}

namespace gpu {
namespace gles2 {
class GLES2Interface;
}
}  // namespace gpu

class VIZ_SERVICE_EXPORT ColorLUTCache {
 public:
  explicit ColorLUTCache(gpu::gles2::GLES2Interface* gl,
                         bool texture_half_float_linear);
  ~ColorLUTCache();

  struct LUT {
    unsigned int texture;
    int size;
  };

  LUT GetLUT(const gfx::ColorTransform* transform);

  // End of frame, assume all LUTs handed out are no longer used.
  void Swap();

 private:
  template <typename T>
  unsigned int MakeLUT(const gfx::ColorTransform* transform, int lut_samples);

  typedef const gfx::ColorTransform* CacheKey;

  struct CacheVal {
    CacheVal(LUT lut, uint32_t last_used_frame)
        : lut(lut), last_used_frame(last_used_frame) {}
    LUT lut;
    uint32_t last_used_frame;
  };

  base::MRUCache<CacheKey, CacheVal> lut_cache_;
  uint32_t current_frame_;
  gpu::gles2::GLES2Interface* gl_;
  bool texture_half_float_linear_;
  DISALLOW_COPY_AND_ASSIGN(ColorLUTCache);
};

#endif  // COMPONENTS_VIZ_SERVICE_DISPLAY_COLOR_LUT_CACHE_H_
