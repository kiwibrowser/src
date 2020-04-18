// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_COMMON_PAGE_ZOOM_H_
#define CONTENT_PUBLIC_COMMON_PAGE_ZOOM_H_

#include "content/common/content_export.h"

namespace content {

// This enum is the parameter to various text/page zoom commands so we know
// what the specific zoom command is.
enum PageZoom {
  PAGE_ZOOM_OUT   = -1,
  PAGE_ZOOM_RESET = 0,
  PAGE_ZOOM_IN    = 1,
};

// The minimum zoom factor permitted for a page. This is an alternative to
// WebView::minTextSizeMultiplier.
CONTENT_EXPORT extern const double kMinimumZoomFactor;

// The maximum zoom factor permitted for a page. This is an alternative to
// WebView::maxTextSizeMultiplier.
CONTENT_EXPORT extern const double kMaximumZoomFactor;

// Epsilon value for comparing two floating-point zoom values. We don't use
// std::numeric_limits<> because it is too precise for zoom values. Zoom
// values lose precision due to factor/level conversions. A value of 0.001
// is precise enough for zoom value comparisons.
CONTENT_EXPORT extern const double kEpsilon;

// Test if two zoom values (either zoom factors or zoom levels) should be
// considered equal.
CONTENT_EXPORT bool ZoomValuesEqual(double value_a, double value_b);

// Converts between zoom factors and levels.
CONTENT_EXPORT double ZoomLevelToZoomFactor(double zoom_level);
CONTENT_EXPORT double ZoomFactorToZoomLevel(double factor);

}  // namespace content

#endif  // CONTENT_PUBLIC_COMMON_PAGE_ZOOM_H_
