// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_PAINT_PAINT_TEXT_BLOB_BUILDER_H_
#define CC_PAINT_PAINT_TEXT_BLOB_BUILDER_H_

#include <vector>

#include "base/macros.h"
#include "cc/paint/paint_export.h"
#include "cc/paint/paint_font.h"
#include "cc/paint/paint_text_blob.h"
#include "third_party/skia/include/core/SkRect.h"
#include "third_party/skia/include/core/SkScalar.h"
#include "third_party/skia/include/core/SkTextBlob.h"

namespace cc {

class CC_PAINT_EXPORT PaintTextBlobBuilder {
 public:
  using RunBuffer = SkTextBlobBuilder::RunBuffer;

  PaintTextBlobBuilder();
  ~PaintTextBlobBuilder();

  scoped_refptr<PaintTextBlob> TakeTextBlob();

  // These functions pass the calls through to SkTextBlobBuilder, see its
  // interface for details.
  const RunBuffer& AllocRunPosH(const PaintFont& font,
                                int count,
                                SkScalar y,
                                const SkRect* bounds = nullptr);

  const RunBuffer& AllocRunPos(const PaintFont& font,
                               int count,
                               const SkRect* bounds = nullptr);

 private:
  std::vector<PaintTypeface> typefaces_;
  SkTextBlobBuilder sk_builder_;

  DISALLOW_COPY_AND_ASSIGN(PaintTextBlobBuilder);
};

}  // namespace cc

#endif  // CC_PAINT_PAINT_TEXT_BLOB_BUILDER_H_
