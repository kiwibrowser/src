/*
 * Copyright (C) 2007 Rob Buis <buis@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_SVG_SVG_VIEW_SPEC_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_SVG_SVG_VIEW_SPEC_H_

#include "third_party/blink/renderer/core/svg/svg_zoom_and_pan.h"
#include "third_party/blink/renderer/platform/heap/handle.h"

namespace blink {

class FloatRect;
class SVGPreserveAspectRatio;
class SVGRect;
class SVGSVGElement;
class SVGTransformList;

class SVGViewSpec final : public GarbageCollectedFinalized<SVGViewSpec>,
                          public SVGZoomAndPan {
 public:
  static SVGViewSpec* CreateForElement(SVGSVGElement&);

  bool ParseViewSpec(const String&);
  void Reset();
  template <typename T>
  void InheritViewAttributesFromElement(T&);

  SVGRect* ViewBox() { return view_box_; }
  SVGPreserveAspectRatio* PreserveAspectRatio() {
    return preserve_aspect_ratio_;
  }
  SVGTransformList* Transform() { return transform_; }

  virtual void Trace(blink::Visitor*);

 private:
  SVGViewSpec();

  template <typename CharType>
  bool ParseViewSpecInternal(const CharType* ptr, const CharType* end);

  void SetViewBox(const FloatRect&);
  void SetPreserveAspectRatio(const SVGPreserveAspectRatio&);

  Member<SVGRect> view_box_;
  Member<SVGPreserveAspectRatio> preserve_aspect_ratio_;
  Member<SVGTransformList> transform_;
};

template <typename T>
void SVGViewSpec::InheritViewAttributesFromElement(T& inherit_from_element) {
  if (inherit_from_element.HasValidViewBox())
    SetViewBox(inherit_from_element.viewBox()->CurrentValue()->Value());

  if (inherit_from_element.preserveAspectRatio()->IsSpecified()) {
    SetPreserveAspectRatio(
        *inherit_from_element.preserveAspectRatio()->CurrentValue());
  }

  if (inherit_from_element.hasAttribute(SVGNames::zoomAndPanAttr))
    setZoomAndPan(inherit_from_element.zoomAndPan());
}

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_SVG_SVG_VIEW_SPEC_H_
