// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_COMPOSITING_DISPLAY_ITEM_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_COMPOSITING_DISPLAY_ITEM_H_

#include "third_party/blink/renderer/platform/geometry/float_rect.h"
#include "third_party/blink/renderer/platform/graphics/graphics_types.h"
#include "third_party/blink/renderer/platform/graphics/paint/display_item.h"
#include "third_party/skia/include/core/SkBlendMode.h"

namespace blink {

class PLATFORM_EXPORT BeginCompositingDisplayItem final
    : public PairedBeginDisplayItem {
 public:
  BeginCompositingDisplayItem(const DisplayItemClient& client,
                              const SkBlendMode xfer_mode,
                              const float opacity,
                              const FloatRect* bounds,
                              ColorFilter color_filter = kColorFilterNone)
      : PairedBeginDisplayItem(client, kBeginCompositing, sizeof(*this)),
        xfer_mode_(xfer_mode),
        opacity_(opacity),
        has_bounds_(bounds),
        color_filter_(color_filter) {
    if (bounds)
      bounds_ = FloatRect(*bounds);
  }

  void Replay(GraphicsContext&) const override;
  void AppendToDisplayItemList(const FloatSize&,
                               cc::DisplayItemList&) const override;

 private:
#if DCHECK_IS_ON()
  void PropertiesAsJSON(JSONObject&) const override;
#endif
  bool Equals(const DisplayItem& other) const final {
    return DisplayItem::Equals(other) &&
           xfer_mode_ == static_cast<const BeginCompositingDisplayItem&>(other)
                             .xfer_mode_ &&
           opacity_ == static_cast<const BeginCompositingDisplayItem&>(other)
                           .opacity_ &&
           has_bounds_ == static_cast<const BeginCompositingDisplayItem&>(other)
                              .has_bounds_ &&
           bounds_ ==
               static_cast<const BeginCompositingDisplayItem&>(other).bounds_ &&
           color_filter_ ==
               static_cast<const BeginCompositingDisplayItem&>(other)
                   .color_filter_;
  }

  const SkBlendMode xfer_mode_;
  const float opacity_;
  bool has_bounds_;
  FloatRect bounds_;
  ColorFilter color_filter_;
};

class PLATFORM_EXPORT EndCompositingDisplayItem final
    : public PairedEndDisplayItem {
 public:
  EndCompositingDisplayItem(const DisplayItemClient& client)
      : PairedEndDisplayItem(client, kEndCompositing, sizeof(*this)) {}

  void Replay(GraphicsContext&) const override;
  void AppendToDisplayItemList(const FloatSize&,
                               cc::DisplayItemList&) const override;

 private:
#if DCHECK_IS_ON()
  bool IsEndAndPairedWith(DisplayItem::Type other_type) const final {
    return other_type == kBeginCompositing;
  }
#endif
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_COMPOSITING_DISPLAY_ITEM_H_
