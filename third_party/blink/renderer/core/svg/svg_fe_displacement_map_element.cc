/*
 * Copyright (C) 2006 Oliver Hunt <oliver@nerget.com>
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

#include "third_party/blink/renderer/core/svg/svg_fe_displacement_map_element.h"

#include "third_party/blink/renderer/core/svg/graphics/filters/svg_filter_builder.h"
#include "third_party/blink/renderer/core/svg_names.h"

namespace blink {

template <>
const SVGEnumerationStringEntries&
GetStaticStringEntries<ChannelSelectorType>() {
  DEFINE_STATIC_LOCAL(SVGEnumerationStringEntries, entries, ());
  if (entries.IsEmpty()) {
    entries.push_back(std::make_pair(CHANNEL_R, "R"));
    entries.push_back(std::make_pair(CHANNEL_G, "G"));
    entries.push_back(std::make_pair(CHANNEL_B, "B"));
    entries.push_back(std::make_pair(CHANNEL_A, "A"));
  }
  return entries;
}

inline SVGFEDisplacementMapElement::SVGFEDisplacementMapElement(
    Document& document)
    : SVGFilterPrimitiveStandardAttributes(SVGNames::feDisplacementMapTag,
                                           document),
      scale_(SVGAnimatedNumber::Create(this,
                                       SVGNames::scaleAttr,
                                       SVGNumber::Create(0))),
      in1_(SVGAnimatedString::Create(this, SVGNames::inAttr)),
      in2_(SVGAnimatedString::Create(this, SVGNames::in2Attr)),
      x_channel_selector_(SVGAnimatedEnumeration<ChannelSelectorType>::Create(
          this,
          SVGNames::xChannelSelectorAttr,
          CHANNEL_A)),
      y_channel_selector_(SVGAnimatedEnumeration<ChannelSelectorType>::Create(
          this,
          SVGNames::yChannelSelectorAttr,
          CHANNEL_A)) {
  AddToPropertyMap(scale_);
  AddToPropertyMap(in1_);
  AddToPropertyMap(in2_);
  AddToPropertyMap(x_channel_selector_);
  AddToPropertyMap(y_channel_selector_);
}

void SVGFEDisplacementMapElement::Trace(blink::Visitor* visitor) {
  visitor->Trace(scale_);
  visitor->Trace(in1_);
  visitor->Trace(in2_);
  visitor->Trace(x_channel_selector_);
  visitor->Trace(y_channel_selector_);
  SVGFilterPrimitiveStandardAttributes::Trace(visitor);
}

DEFINE_NODE_FACTORY(SVGFEDisplacementMapElement)

bool SVGFEDisplacementMapElement::SetFilterEffectAttribute(
    FilterEffect* effect,
    const QualifiedName& attr_name) {
  FEDisplacementMap* displacement_map = static_cast<FEDisplacementMap*>(effect);
  if (attr_name == SVGNames::xChannelSelectorAttr)
    return displacement_map->SetXChannelSelector(
        x_channel_selector_->CurrentValue()->EnumValue());
  if (attr_name == SVGNames::yChannelSelectorAttr)
    return displacement_map->SetYChannelSelector(
        y_channel_selector_->CurrentValue()->EnumValue());
  if (attr_name == SVGNames::scaleAttr)
    return displacement_map->SetScale(scale_->CurrentValue()->Value());

  return SVGFilterPrimitiveStandardAttributes::SetFilterEffectAttribute(
      effect, attr_name);
}

void SVGFEDisplacementMapElement::SvgAttributeChanged(
    const QualifiedName& attr_name) {
  if (attr_name == SVGNames::xChannelSelectorAttr ||
      attr_name == SVGNames::yChannelSelectorAttr ||
      attr_name == SVGNames::scaleAttr) {
    SVGElement::InvalidationGuard invalidation_guard(this);
    PrimitiveAttributeChanged(attr_name);
    return;
  }

  if (attr_name == SVGNames::inAttr || attr_name == SVGNames::in2Attr) {
    SVGElement::InvalidationGuard invalidation_guard(this);
    Invalidate();
    return;
  }

  SVGFilterPrimitiveStandardAttributes::SvgAttributeChanged(attr_name);
}

FilterEffect* SVGFEDisplacementMapElement::Build(
    SVGFilterBuilder* filter_builder,
    Filter* filter) {
  FilterEffect* input1 = filter_builder->GetEffectById(
      AtomicString(in1_->CurrentValue()->Value()));
  FilterEffect* input2 = filter_builder->GetEffectById(
      AtomicString(in2_->CurrentValue()->Value()));
  DCHECK(input1);
  DCHECK(input2);

  FilterEffect* effect = FEDisplacementMap::Create(
      filter, x_channel_selector_->CurrentValue()->EnumValue(),
      y_channel_selector_->CurrentValue()->EnumValue(),
      scale_->CurrentValue()->Value());
  FilterEffectVector& input_effects = effect->InputEffects();
  input_effects.ReserveCapacity(2);
  input_effects.push_back(input1);
  input_effects.push_back(input2);
  return effect;
}

}  // namespace blink
