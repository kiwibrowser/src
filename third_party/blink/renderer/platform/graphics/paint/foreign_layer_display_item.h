// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_FOREIGN_LAYER_DISPLAY_ITEM_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_FOREIGN_LAYER_DISPLAY_ITEM_H_

#include "base/memory/ref_counted.h"
#include "third_party/blink/renderer/platform/graphics/paint/display_item.h"
#include "third_party/blink/renderer/platform/platform_export.h"

namespace cc {
class Layer;
}

namespace blink {

class GraphicsContext;

// Represents foreign content (produced outside Blink) which draws to a layer.
// A client supplies a layer which can be unwrapped and inserted into the full
// layer tree.
//
// Before SPv2, this content is not painted, but is instead inserted into the
// GraphicsLayer tree.
class PLATFORM_EXPORT ForeignLayerDisplayItem final : public DisplayItem {
 public:
  ForeignLayerDisplayItem(const DisplayItemClient&,
                          Type,
                          scoped_refptr<cc::Layer>,
                          const FloatPoint& location,
                          const IntSize& bounds);
  ~ForeignLayerDisplayItem() override;

  cc::Layer* GetLayer() const { return layer_.get(); }
  const FloatPoint& Location() const { return location_; }
  const IntSize& Bounds() const { return bounds_; }

  // DisplayItem
  void Replay(GraphicsContext&) const override;
  void AppendToDisplayItemList(const FloatSize&,
                               cc::DisplayItemList&) const override;
  bool DrawsContent() const override;
  bool Equals(const DisplayItem&) const override;
#if DCHECK_IS_ON()
  void PropertiesAsJSON(JSONObject&) const override;
#endif

 private:
  scoped_refptr<cc::Layer> layer_;
  FloatPoint location_;
  IntSize bounds_;
};

// Records a foreign layer into a GraphicsContext.
// Use this where you would use a recorder class.
PLATFORM_EXPORT void RecordForeignLayer(GraphicsContext&,
                                        const DisplayItemClient&,
                                        DisplayItem::Type,
                                        scoped_refptr<cc::Layer>,
                                        const FloatPoint& location,
                                        const IntSize& bounds);

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_FOREIGN_LAYER_DISPLAY_ITEM_H_
