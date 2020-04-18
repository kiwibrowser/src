// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/paint/paint_font.h"

#include "cc/paint/paint_export.h"
#include "cc/paint/paint_typeface.h"
#include "third_party/skia/include/core/SkPaint.h"

namespace cc {

PaintFont::PaintFont() = default;
PaintFont::~PaintFont() = default;

void PaintFont::SetTextEncoding(SkPaint::TextEncoding encoding) {
  sk_paint_.setTextEncoding(encoding);
}

void PaintFont::SetAntiAlias(bool use_anti_alias) {
  sk_paint_.setAntiAlias(use_anti_alias);
}

void PaintFont::SetHinting(SkPaint::Hinting hinting) {
  sk_paint_.setHinting(hinting);
}

void PaintFont::SetEmbeddedBitmapText(bool use_bitmaps) {
  sk_paint_.setEmbeddedBitmapText(use_bitmaps);
}

void PaintFont::SetAutohinted(bool use_auto_hint) {
  sk_paint_.setAutohinted(use_auto_hint);
}

void PaintFont::SetLcdRenderText(bool lcd_text) {
  sk_paint_.setLCDRenderText(lcd_text);
}

void PaintFont::SetSubpixelText(bool subpixel_text) {
  sk_paint_.setSubpixelText(subpixel_text);
}

void PaintFont::SetTextSize(SkScalar size) {
  sk_paint_.setTextSize(size);
}

void PaintFont::SetTypeface(const PaintTypeface& typeface) {
  typeface_ = typeface;
  sk_paint_.setTypeface(typeface.ToSkTypeface());
}

void PaintFont::SetFakeBoldText(bool bold_text) {
  sk_paint_.setFakeBoldText(bold_text);
}

void PaintFont::SetTextSkewX(SkScalar skew) {
  sk_paint_.setTextSkewX(skew);
}

void PaintFont::SetFlags(uint32_t flags) {
  sk_paint_.setFlags(flags);
}

}  // namespace cc
