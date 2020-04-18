// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_SHAPE_DETECTION_DETECTION_UTILS_MAC_H_
#define SERVICES_SHAPE_DETECTION_DETECTION_UTILS_MAC_H_

#import <CoreImage/CoreImage.h>

#include "base/mac/scoped_nsobject.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/geometry/rect_f.h"

namespace shape_detection {

// Takes a ScopedSharedBufferHandle with dimensions and produces a new CIImage
// with the same contents, or a null scoped_nsobject is something goes wrong.
base::scoped_nsobject<CIImage> CreateCIImageFromSkBitmap(
    const SkBitmap& bitmap);

gfx::RectF ConvertCGToGfxCoordinates(CGRect bounds, int height);

}  // namespace shape_detection

#endif  // SERVICES_SHAPE_DETECTION_DETECTION_UTILS_MAC_H_
