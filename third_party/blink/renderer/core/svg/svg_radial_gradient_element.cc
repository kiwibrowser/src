/*
 * Copyright (C) 2004, 2005, 2006, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007 Rob Buis <buis@kde.org>
 * Copyright (C) 2008 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2008 Dirk Schulze <krit@webkit.org>
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
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

#include "third_party/blink/renderer/core/svg/svg_radial_gradient_element.h"

#include "third_party/blink/renderer/core/layout/svg/layout_svg_resource_radial_gradient.h"
#include "third_party/blink/renderer/core/svg/radial_gradient_attributes.h"

namespace blink {

inline SVGRadialGradientElement::SVGRadialGradientElement(Document& document)
    : SVGGradientElement(SVGNames::radialGradientTag, document),
      cx_(SVGAnimatedLength::Create(this,
                                    SVGNames::cxAttr,
                                    SVGLength::Create(SVGLengthMode::kWidth))),
      cy_(SVGAnimatedLength::Create(this,
                                    SVGNames::cyAttr,
                                    SVGLength::Create(SVGLengthMode::kHeight))),
      r_(SVGAnimatedLength::Create(this,
                                   SVGNames::rAttr,
                                   SVGLength::Create(SVGLengthMode::kOther))),
      fx_(SVGAnimatedLength::Create(this,
                                    SVGNames::fxAttr,
                                    SVGLength::Create(SVGLengthMode::kWidth))),
      fy_(SVGAnimatedLength::Create(this,
                                    SVGNames::fyAttr,
                                    SVGLength::Create(SVGLengthMode::kHeight))),
      fr_(SVGAnimatedLength::Create(this,
                                    SVGNames::frAttr,
                                    SVGLength::Create(SVGLengthMode::kOther))) {
  // Spec: If the cx/cy/r attribute is not specified, the effect is as if a
  // value of "50%" were specified.
  cx_->SetDefaultValueAsString("50%");
  cy_->SetDefaultValueAsString("50%");
  r_->SetDefaultValueAsString("50%");

  // SVG2-Draft Spec: If the fr attributed is not specified, the effect is as if
  // a value of "0%" were specified.
  fr_->SetDefaultValueAsString("0%");

  AddToPropertyMap(cx_);
  AddToPropertyMap(cy_);
  AddToPropertyMap(r_);
  AddToPropertyMap(fx_);
  AddToPropertyMap(fy_);
  AddToPropertyMap(fr_);
}

void SVGRadialGradientElement::Trace(blink::Visitor* visitor) {
  visitor->Trace(cx_);
  visitor->Trace(cy_);
  visitor->Trace(r_);
  visitor->Trace(fx_);
  visitor->Trace(fy_);
  visitor->Trace(fr_);
  SVGGradientElement::Trace(visitor);
}

DEFINE_NODE_FACTORY(SVGRadialGradientElement)

void SVGRadialGradientElement::SvgAttributeChanged(
    const QualifiedName& attr_name) {
  if (attr_name == SVGNames::cxAttr || attr_name == SVGNames::cyAttr ||
      attr_name == SVGNames::fxAttr || attr_name == SVGNames::fyAttr ||
      attr_name == SVGNames::rAttr || attr_name == SVGNames::frAttr) {
    SVGElement::InvalidationGuard invalidation_guard(this);
    UpdateRelativeLengthsInformation();
    InvalidateGradient(LayoutInvalidationReason::kAttributeChanged);
    return;
  }

  SVGGradientElement::SvgAttributeChanged(attr_name);
}

LayoutObject* SVGRadialGradientElement::CreateLayoutObject(
    const ComputedStyle&) {
  return new LayoutSVGResourceRadialGradient(this);
}

static void SetGradientAttributes(const SVGGradientElement& element,
                                  RadialGradientAttributes& attributes,
                                  bool is_radial) {
  element.SynchronizeAnimatedSVGAttribute(AnyQName());
  element.CollectCommonAttributes(attributes);

  if (!is_radial)
    return;
  const SVGRadialGradientElement& radial = ToSVGRadialGradientElement(element);

  if (!attributes.HasCx() && radial.cx()->IsSpecified())
    attributes.SetCx(radial.cx()->CurrentValue());

  if (!attributes.HasCy() && radial.cy()->IsSpecified())
    attributes.SetCy(radial.cy()->CurrentValue());

  if (!attributes.HasR() && radial.r()->IsSpecified())
    attributes.SetR(radial.r()->CurrentValue());

  if (!attributes.HasFx() && radial.fx()->IsSpecified())
    attributes.SetFx(radial.fx()->CurrentValue());

  if (!attributes.HasFy() && radial.fy()->IsSpecified())
    attributes.SetFy(radial.fy()->CurrentValue());

  if (!attributes.HasFr() && radial.fr()->IsSpecified())
    attributes.SetFr(radial.fr()->CurrentValue());
}

bool SVGRadialGradientElement::CollectGradientAttributes(
    RadialGradientAttributes& attributes) {
  DCHECK(GetLayoutObject());

  VisitedSet visited;
  const SVGGradientElement* current = this;

  while (true) {
    SetGradientAttributes(*current, attributes,
                          IsSVGRadialGradientElement(*current));
    visited.insert(current);

    current = current->ReferencedElement();
    if (!current || visited.Contains(current))
      break;
    if (!current->GetLayoutObject())
      return false;
  }

  // Handle default values for fx/fy
  if (!attributes.HasFx())
    attributes.SetFx(attributes.Cx());

  if (!attributes.HasFy())
    attributes.SetFy(attributes.Cy());

  return true;
}

bool SVGRadialGradientElement::SelfHasRelativeLengths() const {
  return cx_->CurrentValue()->IsRelative() ||
         cy_->CurrentValue()->IsRelative() ||
         r_->CurrentValue()->IsRelative() ||
         fx_->CurrentValue()->IsRelative() ||
         fy_->CurrentValue()->IsRelative() || fr_->CurrentValue()->IsRelative();
}

}  // namespace blink
