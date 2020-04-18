// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_SCROLL_DISPLAY_ITEM_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_SCROLL_DISPLAY_ITEM_H_

#include "third_party/blink/renderer/platform/geometry/int_size.h"
#include "third_party/blink/renderer/platform/graphics/paint/display_item.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"

namespace blink {

class PLATFORM_EXPORT BeginScrollDisplayItem final
    : public PairedBeginDisplayItem {
 public:
  BeginScrollDisplayItem(const DisplayItemClient& client,
                         Type type,
                         const IntSize& current_offset)
      : PairedBeginDisplayItem(client, type, sizeof(*this)),
        current_offset_(current_offset) {
    DCHECK(IsScrollType(type));
  }

  void Replay(GraphicsContext&) const override;
  void AppendToDisplayItemList(const FloatSize&,
                               cc::DisplayItemList&) const override;

  const IntSize& CurrentOffset() const { return current_offset_; }

 private:
#if DCHECK_IS_ON()
  void PropertiesAsJSON(JSONObject&) const final;
#endif
  bool Equals(const DisplayItem& other) const final {
    return DisplayItem::Equals(other) &&
           current_offset_ == static_cast<const BeginScrollDisplayItem&>(other)
                                  .current_offset_;
  }

  const IntSize current_offset_;
};

class PLATFORM_EXPORT EndScrollDisplayItem final : public PairedEndDisplayItem {
 public:
  EndScrollDisplayItem(const DisplayItemClient& client, Type type)
      : PairedEndDisplayItem(client, type, sizeof(*this)) {
    DCHECK(IsEndScrollType(type));
  }

  void Replay(GraphicsContext&) const override;
  void AppendToDisplayItemList(const FloatSize&,
                               cc::DisplayItemList&) const override;

 private:
#if DCHECK_IS_ON()
  bool IsEndAndPairedWith(DisplayItem::Type other_type) const final {
    return DisplayItem::IsScrollType(other_type);
  }
#endif
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_SCROLL_DISPLAY_ITEM_H_
