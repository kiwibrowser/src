// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "skia/ext/opacity_filter_canvas.h"
#include "third_party/skia/include/core/SkPaint.h"
#include "third_party/skia/include/core/SkTLazy.h"

namespace skia {

OpacityFilterCanvas::OpacityFilterCanvas(SkCanvas* canvas,
                                         float opacity,
                                         bool disable_image_filtering)
    : INHERITED(canvas),
      alpha_(SkScalarRoundToInt(opacity * 255)),
      disable_image_filtering_(disable_image_filtering) { }

bool OpacityFilterCanvas::onFilter(SkTCopyOnFirstWrite<SkPaint>* paint, Type) const {
  // TODO(fmalita): with the new onFilter() API we could override alpha even
  // when the original paint is null; is this something we should do?
  if (*paint) {
    if (alpha_ < 255)
      paint->writable()->setAlpha(alpha_);

    if (disable_image_filtering_)
      paint->writable()->setFilterQuality(kNone_SkFilterQuality);
  }

  return true;
}

void OpacityFilterCanvas::onDrawPicture(const SkPicture* picture,
                                        const SkMatrix* matrix,
                                        const SkPaint* paint) {
  SkTCopyOnFirstWrite<SkPaint> filteredPaint(paint);
  if (this->onFilter(&filteredPaint, kPicture_Type)) {
    // Unfurl pictures in order to filter nested paints.
    this->SkCanvas::onDrawPicture(picture, matrix, filteredPaint);
  }
}

}  // namespace skia
