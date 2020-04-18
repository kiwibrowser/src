// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/compositor/compositing_recorder.h"

#include "cc/paint/display_item_list.h"
#include "cc/paint/paint_op_buffer.h"
#include "ui/compositor/paint_context.h"
#include "ui/gfx/canvas.h"

namespace ui {

CompositingRecorder::CompositingRecorder(const PaintContext& context,
                                         uint8_t alpha,
                                         bool lcd_text_requires_opaque_layer)
    : context_(context),
      saved_(alpha < 255) {
  if (!saved_)
    return;

  context_.list_->StartPaint();
  context_.list_->push<cc::SaveLayerAlphaOp>(nullptr, alpha,
                                             !lcd_text_requires_opaque_layer);
  context_.list_->EndPaintOfPairedBegin();
}

CompositingRecorder::~CompositingRecorder() {
  if (!saved_)
    return;

  context_.list_->StartPaint();
  context_.list_->push<cc::RestoreOp>();
  context_.list_->EndPaintOfPairedEnd();
}

}  // namespace ui
