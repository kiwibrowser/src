// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/fonts/skia/skia_text_metrics.h"

#include "build/build_config.h"
#include "third_party/blink/renderer/platform/wtf/math_extras.h"

#include <SkPath.h>

namespace blink {

SkiaTextMetrics::SkiaTextMetrics(const SkPaint* paint) : paint_(paint) {
  CHECK(paint_->getTextEncoding() == SkPaint::kGlyphID_TextEncoding);
}

void SkiaTextMetrics::GetGlyphWidthForHarfBuzz(hb_codepoint_t codepoint,
                                               hb_position_t* width) {
  DCHECK_LE(codepoint, 0xFFFFu);
  CHECK(width);

  SkScalar sk_width;
  uint16_t glyph = codepoint;

  paint_->getTextWidths(&glyph, sizeof(glyph), &sk_width, nullptr);
  if (!paint_->isSubpixelText())
    sk_width = SkScalarRoundToInt(sk_width);
  *width = SkiaScalarToHarfBuzzPosition(sk_width);
}

void SkiaTextMetrics::GetGlyphExtentsForHarfBuzz(hb_codepoint_t codepoint,
                                                 hb_glyph_extents_t* extents) {
  DCHECK_LE(codepoint, 0xFFFFu);
  CHECK(extents);

  SkRect sk_bounds;
  uint16_t glyph = codepoint;

  paint_->getTextWidths(&glyph, sizeof(glyph), nullptr, &sk_bounds);
  if (!paint_->isSubpixelText()) {
    // Use roundOut() rather than round() to avoid rendering glyphs
    // outside the visual overflow rect. crbug.com/452914.
    SkIRect ir;
    sk_bounds.roundOut(&ir);
    sk_bounds.set(ir);
  }

  // Invert y-axis because Skia is y-grows-down but we set up HarfBuzz to be
  // y-grows-up.
  extents->x_bearing = SkiaScalarToHarfBuzzPosition(sk_bounds.fLeft);
  extents->y_bearing = SkiaScalarToHarfBuzzPosition(-sk_bounds.fTop);
  extents->width = SkiaScalarToHarfBuzzPosition(sk_bounds.width());
  extents->height = SkiaScalarToHarfBuzzPosition(-sk_bounds.height());
}

void SkiaTextMetrics::GetSkiaBoundsForGlyph(Glyph glyph, SkRect* bounds) {
#if defined(OS_MACOSX)
  // TODO(drott): Remove this once we have better metrics bounds
  // on Mac, https://bugs.chromium.org/p/skia/issues/detail?id=5328
  SkPath path;
  paint_->getTextPath(&glyph, sizeof(glyph), 0, 0, &path);
  *bounds = path.getBounds();
#else
  paint_->getTextWidths(&glyph, sizeof(glyph), nullptr, bounds);
#endif

  if (!paint_->isSubpixelText()) {
    SkIRect ir;
    bounds->roundOut(&ir);
    bounds->set(ir);
  }
}

float SkiaTextMetrics::GetSkiaWidthForGlyph(Glyph glyph) {
  SkScalar sk_width;
  paint_->getTextWidths(&glyph, sizeof(glyph), &sk_width, nullptr);

  if (!paint_->isSubpixelText())
    sk_width = SkScalarRoundToInt(sk_width);

  return SkScalarToFloat(sk_width);
}

hb_position_t SkiaTextMetrics::SkiaScalarToHarfBuzzPosition(SkScalar value) {
  // We treat HarfBuzz hb_position_t as 16.16 fixed-point.
  static const int kHbPosition1 = 1 << 16;
  return clampTo<int>(value * kHbPosition1);
}

}  // namespace blink
