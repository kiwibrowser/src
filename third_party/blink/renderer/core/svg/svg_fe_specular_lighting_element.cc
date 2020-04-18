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

#include "third_party/blink/renderer/core/svg/svg_fe_specular_lighting_element.h"

#include "third_party/blink/renderer/core/layout/layout_object.h"
#include "third_party/blink/renderer/core/style/computed_style.h"
#include "third_party/blink/renderer/core/svg/graphics/filters/svg_filter_builder.h"
#include "third_party/blink/renderer/platform/graphics/filters/fe_specular_lighting.h"
#include "third_party/blink/renderer/platform/graphics/filters/filter.h"

namespace blink {

inline SVGFESpecularLightingElement::SVGFESpecularLightingElement(
    Document& document)
    : SVGFilterPrimitiveStandardAttributes(SVGNames::feSpecularLightingTag,
                                           document),
      specular_constant_(
          SVGAnimatedNumber::Create(this,
                                    SVGNames::specularConstantAttr,
                                    SVGNumber::Create(1))),
      specular_exponent_(
          SVGAnimatedNumber::Create(this,
                                    SVGNames::specularExponentAttr,
                                    SVGNumber::Create(1))),
      surface_scale_(SVGAnimatedNumber::Create(this,
                                               SVGNames::surfaceScaleAttr,
                                               SVGNumber::Create(1))),
      kernel_unit_length_(SVGAnimatedNumberOptionalNumber::Create(
          this,
          SVGNames::kernelUnitLengthAttr)),
      in1_(SVGAnimatedString::Create(this, SVGNames::inAttr)) {
  AddToPropertyMap(specular_constant_);
  AddToPropertyMap(specular_exponent_);
  AddToPropertyMap(surface_scale_);
  AddToPropertyMap(kernel_unit_length_);
  AddToPropertyMap(in1_);
}

void SVGFESpecularLightingElement::Trace(blink::Visitor* visitor) {
  visitor->Trace(specular_constant_);
  visitor->Trace(specular_exponent_);
  visitor->Trace(surface_scale_);
  visitor->Trace(kernel_unit_length_);
  visitor->Trace(in1_);
  SVGFilterPrimitiveStandardAttributes::Trace(visitor);
}

DEFINE_NODE_FACTORY(SVGFESpecularLightingElement)

bool SVGFESpecularLightingElement::SetFilterEffectAttribute(
    FilterEffect* effect,
    const QualifiedName& attr_name) {
  FESpecularLighting* specular_lighting =
      static_cast<FESpecularLighting*>(effect);

  if (attr_name == SVGNames::lighting_colorAttr) {
    LayoutObject* layout_object = this->GetLayoutObject();
    DCHECK(layout_object);
    DCHECK(layout_object->Style());
    return specular_lighting->SetLightingColor(
        layout_object->Style()->SvgStyle().LightingColor());
  }
  if (attr_name == SVGNames::surfaceScaleAttr)
    return specular_lighting->SetSurfaceScale(
        surface_scale_->CurrentValue()->Value());
  if (attr_name == SVGNames::specularConstantAttr)
    return specular_lighting->SetSpecularConstant(
        specular_constant_->CurrentValue()->Value());
  if (attr_name == SVGNames::specularExponentAttr)
    return specular_lighting->SetSpecularExponent(
        specular_exponent_->CurrentValue()->Value());

  LightSource* light_source =
      const_cast<LightSource*>(specular_lighting->GetLightSource());
  SVGFELightElement* light_element = SVGFELightElement::FindLightElement(*this);
  DCHECK(light_source);
  DCHECK(light_element);
  DCHECK(effect->GetFilter());

  if (attr_name == SVGNames::azimuthAttr)
    return light_source->SetAzimuth(
        light_element->azimuth()->CurrentValue()->Value());
  if (attr_name == SVGNames::elevationAttr)
    return light_source->SetElevation(
        light_element->elevation()->CurrentValue()->Value());
  if (attr_name == SVGNames::xAttr || attr_name == SVGNames::yAttr ||
      attr_name == SVGNames::zAttr)
    return light_source->SetPosition(
        effect->GetFilter()->Resolve3dPoint(light_element->GetPosition()));
  if (attr_name == SVGNames::pointsAtXAttr ||
      attr_name == SVGNames::pointsAtYAttr ||
      attr_name == SVGNames::pointsAtZAttr)
    return light_source->SetPointsAt(
        effect->GetFilter()->Resolve3dPoint(light_element->PointsAt()));
  if (attr_name == SVGNames::specularExponentAttr)
    return light_source->SetSpecularExponent(
        light_element->specularExponent()->CurrentValue()->Value());
  if (attr_name == SVGNames::limitingConeAngleAttr)
    return light_source->SetLimitingConeAngle(
        light_element->limitingConeAngle()->CurrentValue()->Value());

  return SVGFilterPrimitiveStandardAttributes::SetFilterEffectAttribute(
      effect, attr_name);
}

void SVGFESpecularLightingElement::SvgAttributeChanged(
    const QualifiedName& attr_name) {
  if (attr_name == SVGNames::surfaceScaleAttr ||
      attr_name == SVGNames::specularConstantAttr ||
      attr_name == SVGNames::specularExponentAttr) {
    SVGElement::InvalidationGuard invalidation_guard(this);
    PrimitiveAttributeChanged(attr_name);
    return;
  }

  if (attr_name == SVGNames::inAttr) {
    SVGElement::InvalidationGuard invalidation_guard(this);
    Invalidate();
    return;
  }

  SVGFilterPrimitiveStandardAttributes::SvgAttributeChanged(attr_name);
}

void SVGFESpecularLightingElement::LightElementAttributeChanged(
    const SVGFELightElement* light_element,
    const QualifiedName& attr_name) {
  if (SVGFELightElement::FindLightElement(*this) != light_element)
    return;

  // The light element has different attribute names so attrName can identify
  // the requested attribute.
  PrimitiveAttributeChanged(attr_name);
}

FilterEffect* SVGFESpecularLightingElement::Build(
    SVGFilterBuilder* filter_builder,
    Filter* filter) {
  FilterEffect* input1 = filter_builder->GetEffectById(
      AtomicString(in1_->CurrentValue()->Value()));
  DCHECK(input1);

  LayoutObject* layout_object = this->GetLayoutObject();
  if (!layout_object)
    return nullptr;

  DCHECK(layout_object->Style());
  Color color = layout_object->Style()->SvgStyle().LightingColor();

  const SVGFELightElement* light_node =
      SVGFELightElement::FindLightElement(*this);
  scoped_refptr<LightSource> light_source =
      light_node ? light_node->GetLightSource(filter) : nullptr;

  FilterEffect* effect = FESpecularLighting::Create(
      filter, color, surface_scale_->CurrentValue()->Value(),
      specular_constant_->CurrentValue()->Value(),
      specular_exponent_->CurrentValue()->Value(), std::move(light_source));
  effect->InputEffects().push_back(input1);
  return effect;
}

}  // namespace blink
