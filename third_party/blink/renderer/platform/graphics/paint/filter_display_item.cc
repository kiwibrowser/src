// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/graphics/paint/filter_display_item.h"

#include "cc/paint/display_item_list.h"
#include "cc/paint/render_surface_filters.h"
#include "third_party/blink/renderer/platform/graphics/graphics_context.h"
#include "ui/gfx/geometry/point_f.h"
#include "ui/gfx/geometry/rect_f.h"

namespace blink {

void BeginFilterDisplayItem::Replay(GraphicsContext& context) const {
  FloatRect image_filter_bounds(bounds_);
  image_filter_bounds.Move(-origin_.X(), -origin_.Y());
  context.Save();
  context.Translate(origin_.X(), origin_.Y());
  context.BeginLayer(1, SkBlendMode::kSrcOver, &image_filter_bounds,
                     kColorFilterNone, image_filter_);
  context.Translate(-origin_.X(), -origin_.Y());
}

void BeginFilterDisplayItem::AppendToDisplayItemList(
    const FloatSize&,
    cc::DisplayItemList& list) const {
  list.StartPaint();

  // TODO(danakj): Skip the save+translate+restore if the origin is 0,0. This
  // should be easier to do when this code is part of the blink DisplayItem
  // which can keep related state.
  list.push<cc::SaveOp>();
  list.push<cc::TranslateOp>(origin_.X(), origin_.Y());

  cc::PaintFlags flags;
  flags.setImageFilter(cc::RenderSurfaceFilters::BuildImageFilter(
      compositor_filter_operations_.AsCcFilterOperations(),
      static_cast<gfx::SizeF>(bounds_.Size())));

  SkRect layer_bounds = bounds_;
  layer_bounds.offset(-origin_.X(), -origin_.Y());
  list.push<cc::SaveLayerOp>(&layer_bounds, &flags);
  list.push<cc::TranslateOp>(-origin_.X(), -origin_.Y());

  list.EndPaintOfPairedBegin(EnclosingIntRect(bounds_));
}

bool BeginFilterDisplayItem::DrawsContent() const {
  // Skia cannot currently tell us if a filter will draw content,
  // even when no input primitives are drawn.
  return true;
}

#if DCHECK_IS_ON()
void BeginFilterDisplayItem::PropertiesAsJSON(JSONObject& json) const {
  DisplayItem::PropertiesAsJSON(json);
  json.SetString("filterBounds", bounds_.ToString());
}
#endif

void EndFilterDisplayItem::Replay(GraphicsContext& context) const {
  context.EndLayer();
  context.Restore();
}

void EndFilterDisplayItem::AppendToDisplayItemList(
    const FloatSize&,
    cc::DisplayItemList& list) const {
  list.StartPaint();
  list.push<cc::RestoreOp>();  // For SaveLayerOp.
  list.push<cc::RestoreOp>();  // For SaveOp.
  list.EndPaintOfPairedEnd();
}

}  // namespace blink
