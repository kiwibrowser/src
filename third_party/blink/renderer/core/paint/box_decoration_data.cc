// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/box_decoration_data.h"

#include "third_party/blink/renderer/core/layout/layout_box.h"
#include "third_party/blink/renderer/core/layout/ng/ng_physical_fragment.h"
#include "third_party/blink/renderer/core/paint/box_painter.h"
#include "third_party/blink/renderer/core/style/border_edge.h"
#include "third_party/blink/renderer/core/style/computed_style.h"
#include "third_party/blink/renderer/platform/graphics/graphics_context.h"

namespace blink {

BoxDecorationData::BoxDecorationData(const LayoutBox& layout_box)
    : BoxDecorationData(layout_box.StyleRef()) {
  bleed_avoidance = ComputeBleedAvoidance(&layout_box);
}

BoxDecorationData::BoxDecorationData(const NGPhysicalFragment& fragment)
    : BoxDecorationData(fragment.Style()) {
  bleed_avoidance = ComputeBleedAvoidance(fragment.GetLayoutObject());
}

BoxDecorationData::BoxDecorationData(const ComputedStyle& style) {
  background_color =
      style.VisitedDependentColor(GetCSSPropertyBackgroundColor());
  has_background = background_color.Alpha() || style.HasBackgroundImage();
  DCHECK(has_background == style.HasBackground());
  has_border_decoration = style.HasBorderDecoration();
  has_appearance = style.HasAppearance();
}

BackgroundBleedAvoidance BoxDecorationData::ComputeBleedAvoidance(
    const LayoutObject* layout_object) {
  DCHECK(layout_object);
  if (layout_object->IsDocumentElement())
    return kBackgroundBleedNone;

  bool background_should_always_be_clipped =
      layout_object->IsBox()
          ? ToLayoutBox(layout_object)->BackgroundShouldAlwaysBeClipped()
          : false;
  return DetermineBackgroundBleedAvoidance(layout_object->GetDocument(),
                                           layout_object->StyleRef(),
                                           background_should_always_be_clipped);
}

namespace {

bool BorderObscuresBackgroundEdge(const ComputedStyle& style) {
  BorderEdge edges[4];
  style.GetBorderEdgeInfo(edges);

  for (auto& edge : edges) {
    if (!edge.ObscuresBackgroundEdge())
      return false;
  }

  return true;
}

}  // anonymous namespace

BackgroundBleedAvoidance BoxDecorationData::DetermineBackgroundBleedAvoidance(
    const Document& document,
    const ComputedStyle& style,
    bool background_should_always_be_clipped) {
  if (!has_background)
    return kBackgroundBleedNone;

  const bool has_border_radius = style.HasBorderRadius();
  if (!has_border_decoration || !has_border_radius ||
      style.CanRenderBorderImage()) {
    if (background_should_always_be_clipped)
      return kBackgroundBleedClipOnly;
    // Border radius clipping may require layer bleed avoidance if we are going
    // to draw an image over something else, because we do not want the
    // antialiasing to lead to bleeding
    if (style.HasBackgroundImage() && has_border_radius) {
      // But if the top layer is opaque for the purposes of background painting,
      // we do not need the bleed avoidance because we will not paint anything
      // behind the top layer.  But only if we need to draw something
      // underneath.
      const FillLayer& fill_layer = style.BackgroundLayers();
      if ((background_color.Alpha() || fill_layer.Next()) &&
          !fill_layer.ImageOccludesNextLayers(document, style))
        return kBackgroundBleedClipLayer;
    }
    return kBackgroundBleedNone;
  }

  if (BorderObscuresBackgroundEdge(style))
    return kBackgroundBleedShrinkBackground;

  return kBackgroundBleedClipLayer;
}

}  // namespace blink
