// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_TRANSFORM_3D_DISPLAY_ITEM_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_TRANSFORM_3D_DISPLAY_ITEM_H_

#include "third_party/blink/renderer/platform/geometry/float_point_3d.h"
#include "third_party/blink/renderer/platform/graphics/paint/display_item.h"
#include "third_party/blink/renderer/platform/transforms/transformation_matrix.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"

namespace blink {

class PLATFORM_EXPORT BeginTransform3DDisplayItem final
    : public PairedBeginDisplayItem {
 public:
  BeginTransform3DDisplayItem(const DisplayItemClient& client,
                              Type type,
                              const TransformationMatrix& transform,
                              const FloatPoint3D& transform_origin)
      : PairedBeginDisplayItem(client, type, sizeof(*this)),
        transform_(transform),
        transform_origin_(transform_origin) {
    DCHECK(DisplayItem::IsTransform3DType(type));
  }

  void Replay(GraphicsContext&) const override;
  void AppendToDisplayItemList(const FloatSize&,
                               cc::DisplayItemList&) const override;

  const TransformationMatrix& Transform() const { return transform_; }
  const FloatPoint3D& TransformOrigin() const { return transform_origin_; }

 private:
#if DCHECK_IS_ON()
  void PropertiesAsJSON(JSONObject&) const final;
#endif
  bool Equals(const DisplayItem& other) const final {
    return DisplayItem::Equals(other) &&
           transform_ == static_cast<const BeginTransform3DDisplayItem&>(other)
                             .transform_ &&
           transform_origin_ ==
               static_cast<const BeginTransform3DDisplayItem&>(other)
                   .transform_origin_;
  }

  const TransformationMatrix transform_;
  const FloatPoint3D transform_origin_;
};

class PLATFORM_EXPORT EndTransform3DDisplayItem final
    : public PairedEndDisplayItem {
 public:
  EndTransform3DDisplayItem(const DisplayItemClient& client, Type type)
      : PairedEndDisplayItem(client, type, sizeof(*this)) {
    DCHECK(DisplayItem::IsEndTransform3DType(type));
  }

  void Replay(GraphicsContext&) const override;
  void AppendToDisplayItemList(const FloatSize&,
                               cc::DisplayItemList&) const override;

 private:
#if DCHECK_IS_ON()
  bool IsEndAndPairedWith(DisplayItem::Type other_type) const final {
    return DisplayItem::Transform3DTypeToEndTransform3DType(other_type) ==
           GetType();
  }
#endif
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_TRANSFORM_3D_DISPLAY_ITEM_H_
