// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "components/viz/common/skia_helper.h"
#include "base/trace_event/trace_event.h"
#include "cc/base/math_util.h"
#include "third_party/skia/include/gpu/GrBackendSurface.h"
#include "ui/gfx/skia_util.h"

namespace viz {
sk_sp<SkImage> SkiaHelper::ApplyImageFilter(sk_sp<SkImage> src_image,
                                            const gfx::RectF& src_rect,
                                            const gfx::RectF& dst_rect,
                                            const gfx::Vector2dF& scale,
                                            sk_sp<SkImageFilter> filter,
                                            SkIPoint* offset,
                                            SkIRect* subset,
                                            const gfx::PointF& origin) {
  if (!filter)
    return nullptr;

  if (!src_image) {
    TRACE_EVENT_INSTANT0("cc",
                         "ApplyImageFilter wrap background texture failed",
                         TRACE_EVENT_SCOPE_THREAD);
    return nullptr;
  }

  // Big filters can sometimes fallback to CPU. Therefore, we need
  // to disable subnormal floats for performance and security reasons.
  cc::ScopedSubnormalFloatDisabler disabler;
  SkMatrix local_matrix;
  local_matrix.setTranslate(origin.x(), origin.y());
  local_matrix.postScale(scale.x(), scale.y());
  local_matrix.postTranslate(-src_rect.x(), -src_rect.y());

  SkIRect clip_bounds = gfx::RectFToSkRect(dst_rect).roundOut();
  clip_bounds.offset(-src_rect.x(), -src_rect.y());

  filter = filter->makeWithLocalMatrix(local_matrix);
  SkIRect in_subset = SkIRect::MakeWH(src_rect.width(), src_rect.height());

  sk_sp<SkImage> image = src_image->makeWithFilter(filter.get(), in_subset,
                                                   clip_bounds, subset, offset);
  if (!image || !image->isTextureBacked()) {
    return nullptr;
  }

  // Force a flush of the Skia pipeline before we switch back to the compositor
  // context.
  image->getBackendTexture(true);
  CHECK(image->isTextureBacked());
  return image;
}

}  // namespace viz
