/*
 * Copyright (C) 2004, 2005, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006 Rob Buis <buis@kde.org>
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

#include "third_party/blink/renderer/core/svg/svg_fe_turbulence_element.h"

#include "third_party/blink/renderer/core/svg_names.h"

namespace blink {

template <>
const SVGEnumerationStringEntries& GetStaticStringEntries<SVGStitchOptions>() {
  DEFINE_STATIC_LOCAL(SVGEnumerationStringEntries, entries, ());
  if (entries.IsEmpty()) {
    entries.push_back(std::make_pair(kSvgStitchtypeStitch, "stitch"));
    entries.push_back(std::make_pair(kSvgStitchtypeNostitch, "noStitch"));
  }
  return entries;
}

template <>
const SVGEnumerationStringEntries& GetStaticStringEntries<TurbulenceType>() {
  DEFINE_STATIC_LOCAL(SVGEnumerationStringEntries, entries, ());
  if (entries.IsEmpty()) {
    entries.push_back(
        std::make_pair(FETURBULENCE_TYPE_FRACTALNOISE, "fractalNoise"));
    entries.push_back(
        std::make_pair(FETURBULENCE_TYPE_TURBULENCE, "turbulence"));
  }
  return entries;
}

inline SVGFETurbulenceElement::SVGFETurbulenceElement(Document& document)
    : SVGFilterPrimitiveStandardAttributes(SVGNames::feTurbulenceTag, document),
      base_frequency_(
          SVGAnimatedNumberOptionalNumber::Create(this,
                                                  SVGNames::baseFrequencyAttr)),
      seed_(SVGAnimatedNumber::Create(this,
                                      SVGNames::seedAttr,
                                      SVGNumber::Create(0))),
      stitch_tiles_(SVGAnimatedEnumeration<SVGStitchOptions>::Create(
          this,
          SVGNames::stitchTilesAttr,
          kSvgStitchtypeNostitch)),
      type_(SVGAnimatedEnumeration<TurbulenceType>::Create(
          this,
          SVGNames::typeAttr,
          FETURBULENCE_TYPE_TURBULENCE)),
      num_octaves_(SVGAnimatedInteger::Create(this,
                                              SVGNames::numOctavesAttr,
                                              SVGInteger::Create(1))) {
  AddToPropertyMap(base_frequency_);
  AddToPropertyMap(seed_);
  AddToPropertyMap(stitch_tiles_);
  AddToPropertyMap(type_);
  AddToPropertyMap(num_octaves_);
}

void SVGFETurbulenceElement::Trace(blink::Visitor* visitor) {
  visitor->Trace(base_frequency_);
  visitor->Trace(seed_);
  visitor->Trace(stitch_tiles_);
  visitor->Trace(type_);
  visitor->Trace(num_octaves_);
  SVGFilterPrimitiveStandardAttributes::Trace(visitor);
}

DEFINE_NODE_FACTORY(SVGFETurbulenceElement)

bool SVGFETurbulenceElement::SetFilterEffectAttribute(
    FilterEffect* effect,
    const QualifiedName& attr_name) {
  FETurbulence* turbulence = static_cast<FETurbulence*>(effect);
  if (attr_name == SVGNames::typeAttr)
    return turbulence->SetType(type_->CurrentValue()->EnumValue());
  if (attr_name == SVGNames::stitchTilesAttr)
    return turbulence->SetStitchTiles(
        stitch_tiles_->CurrentValue()->EnumValue() == kSvgStitchtypeStitch);
  if (attr_name == SVGNames::baseFrequencyAttr) {
    bool base_frequency_x_changed = turbulence->SetBaseFrequencyX(
        baseFrequencyX()->CurrentValue()->Value());
    bool base_frequency_y_changed = turbulence->SetBaseFrequencyY(
        baseFrequencyY()->CurrentValue()->Value());
    return (base_frequency_x_changed || base_frequency_y_changed);
  }
  if (attr_name == SVGNames::seedAttr)
    return turbulence->SetSeed(seed_->CurrentValue()->Value());
  if (attr_name == SVGNames::numOctavesAttr)
    return turbulence->SetNumOctaves(num_octaves_->CurrentValue()->Value());

  return SVGFilterPrimitiveStandardAttributes::SetFilterEffectAttribute(
      effect, attr_name);
}

void SVGFETurbulenceElement::SvgAttributeChanged(
    const QualifiedName& attr_name) {
  if (attr_name == SVGNames::baseFrequencyAttr ||
      attr_name == SVGNames::numOctavesAttr ||
      attr_name == SVGNames::seedAttr ||
      attr_name == SVGNames::stitchTilesAttr ||
      attr_name == SVGNames::typeAttr) {
    SVGElement::InvalidationGuard invalidation_guard(this);
    PrimitiveAttributeChanged(attr_name);
    return;
  }

  SVGFilterPrimitiveStandardAttributes::SvgAttributeChanged(attr_name);
}

FilterEffect* SVGFETurbulenceElement::Build(SVGFilterBuilder*, Filter* filter) {
  return FETurbulence::Create(
      filter, type_->CurrentValue()->EnumValue(),
      baseFrequencyX()->CurrentValue()->Value(),
      baseFrequencyY()->CurrentValue()->Value(),
      num_octaves_->CurrentValue()->Value(), seed_->CurrentValue()->Value(),
      stitch_tiles_->CurrentValue()->EnumValue() == kSvgStitchtypeStitch);
}

}  // namespace blink
