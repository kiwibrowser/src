// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_FILTER_DISPLAY_ITEM_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_FILTER_DISPLAY_ITEM_H_

#include <memory>
#include "base/memory/scoped_refptr.h"
#include "third_party/blink/renderer/platform/geometry/float_rect.h"
#include "third_party/blink/renderer/platform/graphics/compositor_filter_operations.h"
#include "third_party/blink/renderer/platform/graphics/paint/display_item.h"

namespace blink {

class PLATFORM_EXPORT BeginFilterDisplayItem final
    : public PairedBeginDisplayItem {
 public:
  BeginFilterDisplayItem(const DisplayItemClient& client,
                         sk_sp<PaintFilter> image_filter,
                         const FloatRect& bounds,
                         const FloatPoint& origin,
                         CompositorFilterOperations filter_operations)
      : PairedBeginDisplayItem(client, kBeginFilter, sizeof(*this)),
        image_filter_(std::move(image_filter)),
        compositor_filter_operations_(std::move(filter_operations)),
        bounds_(bounds),
        origin_(origin) {}

  void Replay(GraphicsContext&) const override;
  void AppendToDisplayItemList(const FloatSize&,
                               cc::DisplayItemList&) const override;
  bool DrawsContent() const override;

 private:
#if DCHECK_IS_ON()
  void PropertiesAsJSON(JSONObject&) const override;
#endif
  bool Equals(const DisplayItem& other) const final {
    if (!DisplayItem::Equals(other))
      return false;
    const auto& other_item = static_cast<const BeginFilterDisplayItem&>(other);
    // Ignores changes of reference filters because PaintFilter doesn't have
    // an equality operator.
    return bounds_ == other_item.bounds_ && origin_ == other_item.origin_ &&
           compositor_filter_operations_.EqualsIgnoringReferenceFilters(
               other_item.compositor_filter_operations_);
  }

  // FIXME: m_imageFilter should be replaced with m_webFilterOperations when
  // copying data to the compositor.
  sk_sp<PaintFilter> image_filter_;
  CompositorFilterOperations compositor_filter_operations_;
  const FloatRect bounds_;
  const FloatPoint origin_;
};

class PLATFORM_EXPORT EndFilterDisplayItem final : public PairedEndDisplayItem {
 public:
  EndFilterDisplayItem(const DisplayItemClient& client)
      : PairedEndDisplayItem(client, kEndFilter, sizeof(*this)) {}

  void Replay(GraphicsContext&) const override;
  void AppendToDisplayItemList(const FloatSize&,
                               cc::DisplayItemList&) const override;

 private:
#if DCHECK_IS_ON()
  bool IsEndAndPairedWith(DisplayItem::Type other_type) const final {
    return other_type == kBeginFilter;
  }
#endif
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_FILTER_DISPLAY_ITEM_H_
