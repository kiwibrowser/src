// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/paint/paint_text_blob_builder.h"

namespace cc {

PaintTextBlobBuilder::PaintTextBlobBuilder() = default;
PaintTextBlobBuilder::~PaintTextBlobBuilder() = default;

scoped_refptr<PaintTextBlob> PaintTextBlobBuilder::TakeTextBlob() {
  auto result = base::MakeRefCounted<PaintTextBlob>(sk_builder_.make(),
                                                    std::move(typefaces_));
  typefaces_.clear();
  return result;
}

const PaintTextBlobBuilder::RunBuffer& PaintTextBlobBuilder::AllocRunPosH(
    const PaintFont& font,
    int count,
    SkScalar y,
    const SkRect* bounds) {
  typefaces_.push_back(font.typeface());
  return sk_builder_.allocRunPosH(font.ToSkPaint(), count, y, bounds);
}

const PaintTextBlobBuilder::RunBuffer& PaintTextBlobBuilder::AllocRunPos(
    const PaintFont& font,
    int count,
    const SkRect* bounds) {
  typefaces_.push_back(font.typeface());
  return sk_builder_.allocRunPos(font.ToSkPaint(), count, bounds);
}

}  // namespace cc
