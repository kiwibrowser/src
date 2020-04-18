/*
 * Copyright (C) 2009 Dirk Schulze <krit@webkit.org>
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

#include "third_party/blink/renderer/core/svg/svg_fe_convolve_matrix_element.h"

#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/svg/graphics/filters/svg_filter_builder.h"
#include "third_party/blink/renderer/core/svg_names.h"
#include "third_party/blink/renderer/platform/geometry/int_point.h"
#include "third_party/blink/renderer/platform/geometry/int_size.h"

namespace blink {

template <>
const SVGEnumerationStringEntries& GetStaticStringEntries<EdgeModeType>() {
  DEFINE_STATIC_LOCAL(SVGEnumerationStringEntries, entries, ());
  if (entries.IsEmpty()) {
    entries.push_back(std::make_pair(EDGEMODE_DUPLICATE, "duplicate"));
    entries.push_back(std::make_pair(EDGEMODE_WRAP, "wrap"));
    entries.push_back(std::make_pair(EDGEMODE_NONE, "none"));
  }
  return entries;
}

class SVGAnimatedOrder : public SVGAnimatedIntegerOptionalInteger {
 public:
  static SVGAnimatedOrder* Create(SVGElement* context_element) {
    return new SVGAnimatedOrder(context_element);
  }

  SVGParsingError SetBaseValueAsString(const String&) override;

 protected:
  SVGAnimatedOrder(SVGElement* context_element)
      : SVGAnimatedIntegerOptionalInteger(context_element,
                                          SVGNames::orderAttr,
                                          0,
                                          0) {}

  static SVGParsingError CheckValue(SVGParsingError parse_status, int value) {
    if (parse_status != SVGParseStatus::kNoError)
      return parse_status;
    if (value < 0)
      return SVGParseStatus::kNegativeValue;
    if (value == 0)
      return SVGParseStatus::kZeroValue;
    return SVGParseStatus::kNoError;
  }
};

SVGParsingError SVGAnimatedOrder::SetBaseValueAsString(const String& value) {
  SVGParsingError parse_status =
      SVGAnimatedIntegerOptionalInteger::SetBaseValueAsString(value);
  // Check for semantic errors.
  parse_status = CheckValue(parse_status, FirstInteger()->BaseValue()->Value());
  parse_status =
      CheckValue(parse_status, SecondInteger()->BaseValue()->Value());
  return parse_status;
}

inline SVGFEConvolveMatrixElement::SVGFEConvolveMatrixElement(
    Document& document)
    : SVGFilterPrimitiveStandardAttributes(SVGNames::feConvolveMatrixTag,
                                           document),
      bias_(SVGAnimatedNumber::Create(this,
                                      SVGNames::biasAttr,
                                      SVGNumber::Create())),
      divisor_(SVGAnimatedNumber::Create(this,
                                         SVGNames::divisorAttr,
                                         SVGNumber::Create())),
      in1_(SVGAnimatedString::Create(this, SVGNames::inAttr)),
      edge_mode_(
          SVGAnimatedEnumeration<EdgeModeType>::Create(this,
                                                       SVGNames::edgeModeAttr,
                                                       EDGEMODE_DUPLICATE)),
      kernel_matrix_(
          SVGAnimatedNumberList::Create(this, SVGNames::kernelMatrixAttr)),
      kernel_unit_length_(SVGAnimatedNumberOptionalNumber::Create(
          this,
          SVGNames::kernelUnitLengthAttr)),
      order_(SVGAnimatedOrder::Create(this)),
      preserve_alpha_(
          SVGAnimatedBoolean::Create(this, SVGNames::preserveAlphaAttr)),
      target_x_(SVGAnimatedInteger::Create(this,
                                           SVGNames::targetXAttr,
                                           SVGInteger::Create())),
      target_y_(SVGAnimatedInteger::Create(this,
                                           SVGNames::targetYAttr,
                                           SVGInteger::Create())) {
  AddToPropertyMap(preserve_alpha_);
  AddToPropertyMap(divisor_);
  AddToPropertyMap(bias_);
  AddToPropertyMap(kernel_unit_length_);
  AddToPropertyMap(kernel_matrix_);
  AddToPropertyMap(in1_);
  AddToPropertyMap(edge_mode_);
  AddToPropertyMap(order_);
  AddToPropertyMap(target_x_);
  AddToPropertyMap(target_y_);
}

void SVGFEConvolveMatrixElement::Trace(blink::Visitor* visitor) {
  visitor->Trace(bias_);
  visitor->Trace(divisor_);
  visitor->Trace(in1_);
  visitor->Trace(edge_mode_);
  visitor->Trace(kernel_matrix_);
  visitor->Trace(kernel_unit_length_);
  visitor->Trace(order_);
  visitor->Trace(preserve_alpha_);
  visitor->Trace(target_x_);
  visitor->Trace(target_y_);
  SVGFilterPrimitiveStandardAttributes::Trace(visitor);
}

DEFINE_NODE_FACTORY(SVGFEConvolveMatrixElement)

IntSize SVGFEConvolveMatrixElement::MatrixOrder() const {
  if (!order_->IsSpecified())
    return IntSize(3, 3);
  return IntSize(orderX()->CurrentValue()->Value(),
                 orderY()->CurrentValue()->Value());
}

IntPoint SVGFEConvolveMatrixElement::TargetPoint() const {
  IntSize order = MatrixOrder();
  IntPoint target(target_x_->CurrentValue()->Value(),
                  target_y_->CurrentValue()->Value());
  // The spec says the default value is: targetX = floor ( orderX / 2 ))
  if (!target_x_->IsSpecified())
    target.SetX(order.Width() / 2);
  // The spec says the default value is: targetY = floor ( orderY / 2 ))
  if (!target_y_->IsSpecified())
    target.SetY(order.Height() / 2);
  return target;
}

bool SVGFEConvolveMatrixElement::SetFilterEffectAttribute(
    FilterEffect* effect,
    const QualifiedName& attr_name) {
  FEConvolveMatrix* convolve_matrix = static_cast<FEConvolveMatrix*>(effect);
  if (attr_name == SVGNames::edgeModeAttr)
    return convolve_matrix->SetEdgeMode(
        edge_mode_->CurrentValue()->EnumValue());
  if (attr_name == SVGNames::divisorAttr)
    return convolve_matrix->SetDivisor(divisor_->CurrentValue()->Value());
  if (attr_name == SVGNames::biasAttr)
    return convolve_matrix->SetBias(bias_->CurrentValue()->Value());
  if (attr_name == SVGNames::targetXAttr || attr_name == SVGNames::targetYAttr)
    return convolve_matrix->SetTargetOffset(TargetPoint());
  if (attr_name == SVGNames::preserveAlphaAttr)
    return convolve_matrix->SetPreserveAlpha(
        preserve_alpha_->CurrentValue()->Value());
  return SVGFilterPrimitiveStandardAttributes::SetFilterEffectAttribute(
      effect, attr_name);
}

void SVGFEConvolveMatrixElement::SvgAttributeChanged(
    const QualifiedName& attr_name) {
  if (attr_name == SVGNames::edgeModeAttr ||
      attr_name == SVGNames::divisorAttr || attr_name == SVGNames::biasAttr ||
      attr_name == SVGNames::targetXAttr ||
      attr_name == SVGNames::targetYAttr ||
      attr_name == SVGNames::preserveAlphaAttr) {
    SVGElement::InvalidationGuard invalidation_guard(this);
    PrimitiveAttributeChanged(attr_name);
    return;
  }

  if (attr_name == SVGNames::inAttr || attr_name == SVGNames::orderAttr ||
      attr_name == SVGNames::kernelMatrixAttr) {
    SVGElement::InvalidationGuard invalidation_guard(this);
    Invalidate();
    return;
  }

  SVGFilterPrimitiveStandardAttributes::SvgAttributeChanged(attr_name);
}

FilterEffect* SVGFEConvolveMatrixElement::Build(
    SVGFilterBuilder* filter_builder,
    Filter* filter) {
  FilterEffect* input1 = filter_builder->GetEffectById(
      AtomicString(in1_->CurrentValue()->Value()));
  DCHECK(input1);

  float divisor_value = divisor_->CurrentValue()->Value();
  if (!divisor_->IsSpecified()) {
    SVGNumberList* kernel_matrix = kernel_matrix_->CurrentValue();
    size_t kernel_matrix_size = kernel_matrix->length();
    for (size_t i = 0; i < kernel_matrix_size; ++i)
      divisor_value += kernel_matrix->at(i)->Value();
    if (!divisor_value)
      divisor_value = 1;
  }

  FilterEffect* effect = FEConvolveMatrix::Create(
      filter, MatrixOrder(), divisor_value, bias_->CurrentValue()->Value(),
      TargetPoint(), edge_mode_->CurrentValue()->EnumValue(),
      preserve_alpha_->CurrentValue()->Value(),
      kernel_matrix_->CurrentValue()->ToFloatVector());
  effect->InputEffects().push_back(input1);
  return effect;
}

}  // namespace blink
