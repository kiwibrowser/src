// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_COMPOSITOR_COMPOSITING_RECORDER_H_
#define UI_COMPOSITOR_COMPOSITING_RECORDER_H_

#include <stdint.h>

#include "base/macros.h"
#include "ui/compositor/compositor_export.h"

namespace ui {
class PaintContext;

// A class to provide scoped compositing filters (eg opacity) of painting to a
// DisplayItemList. The filters provided will be applied to any
// DisplayItems added to the DisplayItemList while this object is alive. In
// other words, any nested PaintRecorders or other such Recorders will
// be filtered by the effect.
class COMPOSITOR_EXPORT CompositingRecorder {
 public:
  // |alpha| is a value between 0 and 255, where 0 is transparent and 255 is
  // opaque. |size_in_context| is the size in the |context|'s space surrounding
  // everything that's visible.  |lcd_text_requires_opaque_layer| should
  // normally be true; if this is false, Skia will respect text rendering
  // requests for LCD AA even if they occur on non-opaque layers.  This should
  // only be used in cases where the text is known to be rendered opaquely on an
  // opaque background before compositing.
  CompositingRecorder(const PaintContext& context,
                      uint8_t alpha,
                      bool lcd_text_requires_opaque_layer);
  ~CompositingRecorder();

 private:
  const PaintContext& context_;
  bool saved_;

  DISALLOW_COPY_AND_ASSIGN(CompositingRecorder);
};

}  // namespace ui

#endif  // UI_COMPOSITOR_CLIP_TRANSFORM_RECORDER_H_
