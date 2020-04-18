/*
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 Nikolas Zimmermann
 * <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007 Rob Buis <buis@kde.org>
 * Copyright (C) Research In Motion Limited 2009-2010. All rights reserved.
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

#include "third_party/blink/renderer/core/svg/svg_marker_element.h"

#include "third_party/blink/renderer/core/layout/svg/layout_svg_resource_marker.h"
#include "third_party/blink/renderer/core/svg/svg_angle_tear_off.h"
#include "third_party/blink/renderer/core/svg_names.h"

namespace blink {

template <>
const SVGEnumerationStringEntries&
GetStaticStringEntries<SVGMarkerUnitsType>() {
  DEFINE_STATIC_LOCAL(SVGEnumerationStringEntries, entries, ());
  if (entries.IsEmpty()) {
    entries.push_back(
        std::make_pair(kSVGMarkerUnitsUserSpaceOnUse, "userSpaceOnUse"));
    entries.push_back(
        std::make_pair(kSVGMarkerUnitsStrokeWidth, "strokeWidth"));
  }
  return entries;
}

inline SVGMarkerElement::SVGMarkerElement(Document& document)
    : SVGElement(SVGNames::markerTag, document),
      SVGFitToViewBox(this),
      ref_x_(
          SVGAnimatedLength::Create(this,
                                    SVGNames::refXAttr,
                                    SVGLength::Create(SVGLengthMode::kWidth))),
      ref_y_(
          SVGAnimatedLength::Create(this,
                                    SVGNames::refYAttr,
                                    SVGLength::Create(SVGLengthMode::kHeight))),
      marker_width_(
          SVGAnimatedLength::Create(this,
                                    SVGNames::markerWidthAttr,
                                    SVGLength::Create(SVGLengthMode::kWidth))),
      marker_height_(
          SVGAnimatedLength::Create(this,
                                    SVGNames::markerHeightAttr,
                                    SVGLength::Create(SVGLengthMode::kHeight))),
      orient_angle_(SVGAnimatedAngle::Create(this)),
      marker_units_(SVGAnimatedEnumeration<SVGMarkerUnitsType>::Create(
          this,
          SVGNames::markerUnitsAttr,
          kSVGMarkerUnitsStrokeWidth)) {
  // Spec: If the markerWidth/markerHeight attribute is not specified, the
  // effect is as if a value of "3" were specified.
  marker_width_->SetDefaultValueAsString("3");
  marker_height_->SetDefaultValueAsString("3");

  AddToPropertyMap(ref_x_);
  AddToPropertyMap(ref_y_);
  AddToPropertyMap(marker_width_);
  AddToPropertyMap(marker_height_);
  AddToPropertyMap(orient_angle_);
  AddToPropertyMap(marker_units_);
}

void SVGMarkerElement::Trace(blink::Visitor* visitor) {
  visitor->Trace(ref_x_);
  visitor->Trace(ref_y_);
  visitor->Trace(marker_width_);
  visitor->Trace(marker_height_);
  visitor->Trace(orient_angle_);
  visitor->Trace(marker_units_);
  SVGElement::Trace(visitor);
  SVGFitToViewBox::Trace(visitor);
}

DEFINE_NODE_FACTORY(SVGMarkerElement)

AffineTransform SVGMarkerElement::ViewBoxToViewTransform(
    float view_width,
    float view_height) const {
  return SVGFitToViewBox::ViewBoxToViewTransform(
      viewBox()->CurrentValue()->Value(), preserveAspectRatio()->CurrentValue(),
      view_width, view_height);
}

void SVGMarkerElement::SvgAttributeChanged(const QualifiedName& attr_name) {
  bool viewbox_attribute_changed = SVGFitToViewBox::IsKnownAttribute(attr_name);
  bool length_attribute_changed = attr_name == SVGNames::refXAttr ||
                                  attr_name == SVGNames::refYAttr ||
                                  attr_name == SVGNames::markerWidthAttr ||
                                  attr_name == SVGNames::markerHeightAttr;
  if (length_attribute_changed)
    UpdateRelativeLengthsInformation();

  if (viewbox_attribute_changed || length_attribute_changed ||
      attr_name == SVGNames::markerUnitsAttr ||
      attr_name == SVGNames::orientAttr) {
    SVGElement::InvalidationGuard invalidation_guard(this);
    auto* resource_container = ToLayoutSVGResourceContainer(GetLayoutObject());
    if (resource_container) {
      // The marker transform depends on both viewbox attributes, and the marker
      // size attributes (width, height).
      if (viewbox_attribute_changed || length_attribute_changed)
        resource_container->SetNeedsTransformUpdate();
      resource_container->InvalidateCacheAndMarkForLayout();
    }

    return;
  }

  SVGElement::SvgAttributeChanged(attr_name);
}

void SVGMarkerElement::ChildrenChanged(const ChildrenChange& change) {
  SVGElement::ChildrenChanged(change);

  if (change.by_parser)
    return;

  if (LayoutObject* object = GetLayoutObject())
    object->SetNeedsLayoutAndFullPaintInvalidation(
        LayoutInvalidationReason::kChildChanged);
}

void SVGMarkerElement::setOrientToAuto() {
  setAttribute(SVGNames::orientAttr, "auto");
}

void SVGMarkerElement::setOrientToAngle(SVGAngleTearOff* angle) {
  DCHECK(angle);
  SVGAngle* target = angle->Target();
  setAttribute(SVGNames::orientAttr, AtomicString(target->ValueAsString()));
}

LayoutObject* SVGMarkerElement::CreateLayoutObject(const ComputedStyle&) {
  return new LayoutSVGResourceMarker(this);
}

bool SVGMarkerElement::SelfHasRelativeLengths() const {
  return ref_x_->CurrentValue()->IsRelative() ||
         ref_y_->CurrentValue()->IsRelative() ||
         marker_width_->CurrentValue()->IsRelative() ||
         marker_height_->CurrentValue()->IsRelative();
}

bool SVGMarkerElement::LayoutObjectIsNeeded(const ComputedStyle&) const {
  return IsValid() && HasSVGParent();
}

}  // namespace blink
