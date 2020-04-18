/*
 * Copyright (C) Research In Motion Limited 2011. All rights reserved.
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

#include "third_party/blink/renderer/core/svg/svg_fe_drop_shadow_element.h"

#include "third_party/blink/renderer/core/layout/layout_object.h"
#include "third_party/blink/renderer/core/style/computed_style.h"
#include "third_party/blink/renderer/core/style/svg_computed_style.h"
#include "third_party/blink/renderer/core/svg/graphics/filters/svg_filter_builder.h"
#include "third_party/blink/renderer/core/svg_names.h"
#include "third_party/blink/renderer/platform/graphics/filters/fe_drop_shadow.h"

namespace blink {

inline SVGFEDropShadowElement::SVGFEDropShadowElement(Document& document)
    : SVGFilterPrimitiveStandardAttributes(SVGNames::feDropShadowTag, document),
      dx_(SVGAnimatedNumber::Create(this,
                                    SVGNames::dxAttr,
                                    SVGNumber::Create(2))),
      dy_(SVGAnimatedNumber::Create(this,
                                    SVGNames::dyAttr,
                                    SVGNumber::Create(2))),
      std_deviation_(
          SVGAnimatedNumberOptionalNumber::Create(this,
                                                  SVGNames::stdDeviationAttr,
                                                  2,
                                                  2)),
      in1_(SVGAnimatedString::Create(this, SVGNames::inAttr)) {
  AddToPropertyMap(dx_);
  AddToPropertyMap(dy_);
  AddToPropertyMap(std_deviation_);
  AddToPropertyMap(in1_);
}

void SVGFEDropShadowElement::Trace(blink::Visitor* visitor) {
  visitor->Trace(dx_);
  visitor->Trace(dy_);
  visitor->Trace(std_deviation_);
  visitor->Trace(in1_);
  SVGFilterPrimitiveStandardAttributes::Trace(visitor);
}

DEFINE_NODE_FACTORY(SVGFEDropShadowElement)

void SVGFEDropShadowElement::setStdDeviation(float x, float y) {
  stdDeviationX()->BaseValue()->SetValue(x);
  stdDeviationY()->BaseValue()->SetValue(y);
  Invalidate();
}

bool SVGFEDropShadowElement::SetFilterEffectAttribute(
    FilterEffect* effect,
    const QualifiedName& attr_name) {
  DCHECK(GetLayoutObject());
  FEDropShadow* drop_shadow = static_cast<FEDropShadow*>(effect);

  const SVGComputedStyle& svg_style = GetLayoutObject()->StyleRef().SvgStyle();
  if (attr_name == SVGNames::flood_colorAttr) {
    drop_shadow->SetShadowColor(svg_style.FloodColor());
    return true;
  }
  if (attr_name == SVGNames::flood_opacityAttr) {
    drop_shadow->SetShadowOpacity(svg_style.FloodOpacity());
    return true;
  }
  return SVGFilterPrimitiveStandardAttributes::SetFilterEffectAttribute(
      effect, attr_name);
}

void SVGFEDropShadowElement::SvgAttributeChanged(
    const QualifiedName& attr_name) {
  if (attr_name == SVGNames::inAttr ||
      attr_name == SVGNames::stdDeviationAttr ||
      attr_name == SVGNames::dxAttr || attr_name == SVGNames::dyAttr) {
    SVGElement::InvalidationGuard invalidation_guard(this);
    Invalidate();
    return;
  }

  SVGFilterPrimitiveStandardAttributes::SvgAttributeChanged(attr_name);
}

FilterEffect* SVGFEDropShadowElement::Build(SVGFilterBuilder* filter_builder,
                                            Filter* filter) {
  LayoutObject* layout_object = this->GetLayoutObject();
  if (!layout_object)
    return nullptr;

  DCHECK(layout_object->Style());
  const SVGComputedStyle& svg_style = layout_object->Style()->SvgStyle();

  Color color = svg_style.FloodColor();
  float opacity = svg_style.FloodOpacity();

  FilterEffect* input1 = filter_builder->GetEffectById(
      AtomicString(in1_->CurrentValue()->Value()));
  DCHECK(input1);

  // Clamp std.dev. to non-negative. (See SVGFEGaussianBlurElement::build)
  float std_dev_x = std::max(0.0f, stdDeviationX()->CurrentValue()->Value());
  float std_dev_y = std::max(0.0f, stdDeviationY()->CurrentValue()->Value());
  FilterEffect* effect = FEDropShadow::Create(
      filter, std_dev_x, std_dev_y, dx_->CurrentValue()->Value(),
      dy_->CurrentValue()->Value(), color, opacity);
  effect->InputEffects().push_back(input1);
  return effect;
}

}  // namespace blink
