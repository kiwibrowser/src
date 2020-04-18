/*
 * Copyright (C) 2004, 2005, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006 Rob Buis <buis@kde.org>
 * Copyright (C) 2005 Oliver Hunt <oliver@nerget.com>
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

#include "third_party/blink/renderer/core/svg/svg_fe_light_element.h"

#include "third_party/blink/renderer/core/dom/element_traversal.h"
#include "third_party/blink/renderer/core/layout/layout_object.h"
#include "third_party/blink/renderer/core/svg/svg_fe_diffuse_lighting_element.h"
#include "third_party/blink/renderer/core/svg/svg_fe_specular_lighting_element.h"
#include "third_party/blink/renderer/core/svg_names.h"

namespace blink {

SVGFELightElement::SVGFELightElement(const QualifiedName& tag_name,
                                     Document& document)
    : SVGElement(tag_name, document),
      azimuth_(SVGAnimatedNumber::Create(this,
                                         SVGNames::azimuthAttr,
                                         SVGNumber::Create())),
      elevation_(SVGAnimatedNumber::Create(this,
                                           SVGNames::elevationAttr,
                                           SVGNumber::Create())),
      x_(SVGAnimatedNumber::Create(this, SVGNames::xAttr, SVGNumber::Create())),
      y_(SVGAnimatedNumber::Create(this, SVGNames::yAttr, SVGNumber::Create())),
      z_(SVGAnimatedNumber::Create(this, SVGNames::zAttr, SVGNumber::Create())),
      points_at_x_(SVGAnimatedNumber::Create(this,
                                             SVGNames::pointsAtXAttr,
                                             SVGNumber::Create())),
      points_at_y_(SVGAnimatedNumber::Create(this,
                                             SVGNames::pointsAtYAttr,
                                             SVGNumber::Create())),
      points_at_z_(SVGAnimatedNumber::Create(this,
                                             SVGNames::pointsAtZAttr,
                                             SVGNumber::Create())),
      specular_exponent_(
          SVGAnimatedNumber::Create(this,
                                    SVGNames::specularExponentAttr,
                                    SVGNumber::Create(1))),
      limiting_cone_angle_(
          SVGAnimatedNumber::Create(this,
                                    SVGNames::limitingConeAngleAttr,
                                    SVGNumber::Create())) {
  AddToPropertyMap(azimuth_);
  AddToPropertyMap(elevation_);
  AddToPropertyMap(x_);
  AddToPropertyMap(y_);
  AddToPropertyMap(z_);
  AddToPropertyMap(points_at_x_);
  AddToPropertyMap(points_at_y_);
  AddToPropertyMap(points_at_z_);
  AddToPropertyMap(specular_exponent_);
  AddToPropertyMap(limiting_cone_angle_);
}

void SVGFELightElement::Trace(blink::Visitor* visitor) {
  visitor->Trace(azimuth_);
  visitor->Trace(elevation_);
  visitor->Trace(x_);
  visitor->Trace(y_);
  visitor->Trace(z_);
  visitor->Trace(points_at_x_);
  visitor->Trace(points_at_y_);
  visitor->Trace(points_at_z_);
  visitor->Trace(specular_exponent_);
  visitor->Trace(limiting_cone_angle_);
  SVGElement::Trace(visitor);
}

SVGFELightElement* SVGFELightElement::FindLightElement(
    const SVGElement& svg_element) {
  return Traversal<SVGFELightElement>::FirstChild(svg_element);
}

FloatPoint3D SVGFELightElement::GetPosition() const {
  return FloatPoint3D(x()->CurrentValue()->Value(),
                      y()->CurrentValue()->Value(),
                      z()->CurrentValue()->Value());
}

FloatPoint3D SVGFELightElement::PointsAt() const {
  return FloatPoint3D(pointsAtX()->CurrentValue()->Value(),
                      pointsAtY()->CurrentValue()->Value(),
                      pointsAtZ()->CurrentValue()->Value());
}

void SVGFELightElement::SvgAttributeChanged(const QualifiedName& attr_name) {
  if (attr_name == SVGNames::azimuthAttr ||
      attr_name == SVGNames::elevationAttr || attr_name == SVGNames::xAttr ||
      attr_name == SVGNames::yAttr || attr_name == SVGNames::zAttr ||
      attr_name == SVGNames::pointsAtXAttr ||
      attr_name == SVGNames::pointsAtYAttr ||
      attr_name == SVGNames::pointsAtZAttr ||
      attr_name == SVGNames::specularExponentAttr ||
      attr_name == SVGNames::limitingConeAngleAttr) {
    ContainerNode* parent = parentNode();
    if (!parent)
      return;

    LayoutObject* layout_object = parent->GetLayoutObject();
    if (!layout_object || !layout_object->IsSVGResourceFilterPrimitive())
      return;

    SVGElement::InvalidationGuard invalidation_guard(this);
    if (auto* diffuse = ToSVGFEDiffuseLightingElementOrNull(*parent))
      diffuse->LightElementAttributeChanged(this, attr_name);
    else if (auto* specular = ToSVGFESpecularLightingElementOrNull(*parent))
      specular->LightElementAttributeChanged(this, attr_name);

    return;
  }

  SVGElement::SvgAttributeChanged(attr_name);
}

void SVGFELightElement::ChildrenChanged(const ChildrenChange& change) {
  SVGElement::ChildrenChanged(change);

  if (!change.by_parser) {
    if (ContainerNode* parent = parentNode()) {
      LayoutObject* layout_object = parent->GetLayoutObject();
      if (layout_object && layout_object->IsSVGResourceFilterPrimitive())
        MarkForLayoutAndParentResourceInvalidation(*layout_object);
    }
  }
}

}  // namespace blink
