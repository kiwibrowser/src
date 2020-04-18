// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_FILTER_PAINTER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_FILTER_PAINTER_H_

#include <memory>
#include "third_party/blink/renderer/core/paint/paint_layer_painting_info.h"
#include "third_party/blink/renderer/platform/graphics/filters/paint_filter_builder.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"

namespace blink {

class ClipRect;
class GraphicsContext;
class PaintLayer;
class LayerClipRecorder;
class LayoutObject;

class FilterPainter {
 public:
  FilterPainter(PaintLayer&,
                GraphicsContext&,
                const LayoutPoint& offset_from_root,
                const ClipRect&,
                PaintLayerPaintingInfo&,
                PaintLayerFlags paint_flags);
  ~FilterPainter();

  // Returns whether it's ok to clip this PaintLayer's painted outputs
  // the dirty rect. Some filters require input from outside this rect, in
  // which case this method would return true.
  static sk_sp<PaintFilter> GetImageFilter(PaintLayer&);

 private:
  bool filter_in_progress_;
  GraphicsContext& context_;
  std::unique_ptr<LayerClipRecorder> clip_recorder_;
  LayoutObject& layout_object_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_FILTER_PAINTER_H_
