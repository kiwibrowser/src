// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/ng/list/layout_ng_list_marker_image.h"
#include "third_party/blink/renderer/core/svg/graphics/svg_image.h"

namespace blink {

LayoutNGListMarkerImage::LayoutNGListMarkerImage(Element* element)
    : LayoutImage(element) {}

LayoutNGListMarkerImage* LayoutNGListMarkerImage::CreateAnonymous(
    Document* document) {
  LayoutNGListMarkerImage* object = new LayoutNGListMarkerImage(nullptr);
  object->SetDocumentForAnonymous(document);
  return object;
}

bool LayoutNGListMarkerImage::IsOfType(LayoutObjectType type) const {
  return type == kLayoutObjectNGListMarkerImage || LayoutImage::IsOfType(type);
}

// Use default_size(ascent/2) to compute ConcreteObjectSize as svg's intrinsic
// size. Otherwise the width of svg marker will be out of control.
void LayoutNGListMarkerImage::ComputeSVGIntrinsicSizingInfoByDefaultSize(
    IntrinsicSizingInfo& intrinsic_sizing_info) const {
  DCHECK(CachedImage());
  Image* image = CachedImage()->GetImage();
  DCHECK(image && image->IsSVGImage());

  const SimpleFontData* font_data = Style()->GetFont().PrimaryFont();
  DCHECK(font_data);
  if (!font_data)
    return;

  LayoutUnit bullet_width =
      font_data->GetFontMetrics().Ascent() / LayoutUnit(2);
  FloatSize default_size(bullet_width, bullet_width);
  default_size.Scale(1 / Style()->EffectiveZoom());
  FloatSize concrete_size = ToSVGImage(image)->ConcreteObjectSize(default_size);
  concrete_size.Scale(Style()->EffectiveZoom() * ImageDevicePixelRatio());
  LayoutSize svg_image_size(RoundedLayoutSize(concrete_size));

  intrinsic_sizing_info.size.SetWidth(svg_image_size.Width());
  intrinsic_sizing_info.size.SetHeight(svg_image_size.Height());
  intrinsic_sizing_info.has_width = true;
  intrinsic_sizing_info.has_height = true;
}

void LayoutNGListMarkerImage::ComputeIntrinsicSizingInfo(
    IntrinsicSizingInfo& intrinsic_sizing_info) const {
  LayoutImage::ComputeIntrinsicSizingInfo(intrinsic_sizing_info);

  // If this is an SVG image without intrinsic width and height,
  // compute an intrinsic size using the concrete object size resolved
  // with a default object size of 1em x 1em.
  // TODO(fs): Apply this more generally to all images (CSS <image> values)
  // that have no intrinsic width or height. I.e just always compute the
  // concrete object size here.
  if (intrinsic_sizing_info.size.IsEmpty() && CachedImage()) {
    Image* image = CachedImage()->GetImage();
    if (image && image->IsSVGImage())
      ComputeSVGIntrinsicSizingInfoByDefaultSize(intrinsic_sizing_info);
  }
}

}  // namespace blink
