// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_PAINT_PAINT_FONT_H_
#define CC_PAINT_PAINT_FONT_H_

#include "cc/paint/paint_export.h"
#include "cc/paint/paint_typeface.h"
#include "third_party/skia/include/core/SkPaint.h"

namespace cc {

class CC_PAINT_EXPORT PaintFont {
 public:
  PaintFont();
  ~PaintFont();

  void SetTextEncoding(SkPaint::TextEncoding encoding);
  void SetAntiAlias(bool use_anti_alias);
  void SetHinting(SkPaint::Hinting hinting);
  void SetEmbeddedBitmapText(bool use_bitmaps);
  void SetAutohinted(bool use_auto_hint);
  void SetLcdRenderText(bool lcd_text);
  void SetSubpixelText(bool subpixel_text);
  void SetTextSize(SkScalar size);
  void SetTypeface(const PaintTypeface& typeface);
  void SetFakeBoldText(bool bold_text);
  void SetTextSkewX(SkScalar skew);
  void SetFlags(uint32_t flags);

  uint32_t flags() const { return sk_paint_.getFlags(); }

  const PaintTypeface& typeface() const { return typeface_; }
  const SkPaint& ToSkPaint() const { return sk_paint_; }

 private:
  PaintTypeface typeface_;
  SkPaint sk_paint_;
};

}  // namespace cc

#endif  // CC_PAINT_PAINT_FONT_H_
