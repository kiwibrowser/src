// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GFX_SKIA_UTIL_H_
#define UI_GFX_SKIA_UTIL_H_

#include <string>
#include <vector>

#include "third_party/skia/include/core/SkColor.h"
#include "third_party/skia/include/core/SkRect.h"
#include "ui/gfx/geometry/quad_f.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/gfx_export.h"

class SkBitmap;
class SkMatrix;

namespace gfx {

class Point;
class PointF;
class Rect;
class RectF;
class Transform;

// Convert between Skia and gfx types.
GFX_EXPORT SkPoint PointToSkPoint(const Point& point);
GFX_EXPORT SkIPoint PointToSkIPoint(const Point& point);
GFX_EXPORT SkPoint PointFToSkPoint(const PointF& point);
GFX_EXPORT SkRect RectToSkRect(const Rect& rect);
GFX_EXPORT SkIRect RectToSkIRect(const Rect& rect);
GFX_EXPORT Rect SkIRectToRect(const SkIRect& rect);
GFX_EXPORT SkRect RectFToSkRect(const RectF& rect);
GFX_EXPORT RectF SkRectToRectF(const SkRect& rect);
GFX_EXPORT SkSize SizeFToSkSize(const SizeF& size);
GFX_EXPORT SkISize SizeToSkISize(const Size& size);
GFX_EXPORT SizeF SkSizeToSizeF(const SkSize& size);
GFX_EXPORT Size SkISizeToSize(const SkISize& size);

GFX_EXPORT void QuadFToSkPoints(const gfx::QuadF& quad, SkPoint points[4]);

GFX_EXPORT void TransformToFlattenedSkMatrix(const gfx::Transform& transform,
                                             SkMatrix* flattened);

// Returns true if the two bitmaps contain the same pixels.
GFX_EXPORT bool BitmapsAreEqual(const SkBitmap& bitmap1,
                                const SkBitmap& bitmap2);

// Converts Skia ARGB format pixels in |skia| to RGBA.
GFX_EXPORT void ConvertSkiaToRGBA(const unsigned char* skia,
                                  int pixel_width,
                                  unsigned char* rgba);

// Converts a Skia floating-point value to an int appropriate for hb_position_t.
GFX_EXPORT int SkiaScalarToHarfBuzzUnits(SkScalar value);

// Converts an hb_position_t to a Skia floating-point value.
GFX_EXPORT SkScalar HarfBuzzUnitsToSkiaScalar(int value);

// Converts an hb_position_t to a float.
GFX_EXPORT float HarfBuzzUnitsToFloat(int value);

}  // namespace gfx

#endif  // UI_GFX_SKIA_UTIL_H_
