// Copyright 2016 the Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/cssom/computed_style_property_map.h"

#include "third_party/blink/renderer/core/css/computed_style_css_value_mapping.h"
#include "third_party/blink/renderer/core/css/css_custom_property_declaration.h"
#include "third_party/blink/renderer/core/css/css_function_value.h"
#include "third_party/blink/renderer/core/css/css_identifier_value.h"
#include "third_party/blink/renderer/core/css/css_variable_data.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/pseudo_element.h"
#include "third_party/blink/renderer/core/style/computed_style.h"
#include "third_party/blink/renderer/platform/transforms/matrix_3d_transform_operation.h"
#include "third_party/blink/renderer/platform/transforms/matrix_transform_operation.h"
#include "third_party/blink/renderer/platform/transforms/perspective_transform_operation.h"
#include "third_party/blink/renderer/platform/transforms/skew_transform_operation.h"

namespace blink {

namespace {

// We collapse functions like translateX into translate, since we will reify
// them as a translate anyway.
const CSSValue* ComputedTransformComponent(const TransformOperation& operation,
                                           float zoom) {
  switch (operation.GetType()) {
    case TransformOperation::kScaleX:
    case TransformOperation::kScaleY:
    case TransformOperation::kScaleZ:
    case TransformOperation::kScale:
    case TransformOperation::kScale3D: {
      const auto& scale = ToScaleTransformOperation(operation);
      CSSFunctionValue* result = CSSFunctionValue::Create(
          operation.Is3DOperation() ? CSSValueScale3d : CSSValueScale);
      result->Append(*CSSPrimitiveValue::Create(
          scale.X(), CSSPrimitiveValue::UnitType::kNumber));
      result->Append(*CSSPrimitiveValue::Create(
          scale.Y(), CSSPrimitiveValue::UnitType::kNumber));
      if (operation.Is3DOperation()) {
        result->Append(*CSSPrimitiveValue::Create(
            scale.Z(), CSSPrimitiveValue::UnitType::kNumber));
      }
      return result;
    }
    case TransformOperation::kTranslateX:
    case TransformOperation::kTranslateY:
    case TransformOperation::kTranslateZ:
    case TransformOperation::kTranslate:
    case TransformOperation::kTranslate3D: {
      const auto& translate = ToTranslateTransformOperation(operation);
      CSSFunctionValue* result = CSSFunctionValue::Create(
          operation.Is3DOperation() ? CSSValueTranslate3d : CSSValueTranslate);
      result->Append(*CSSPrimitiveValue::Create(translate.X(), zoom));
      result->Append(*CSSPrimitiveValue::Create(translate.Y(), zoom));
      if (operation.Is3DOperation()) {
        result->Append(*CSSPrimitiveValue::Create(
            translate.Z(), CSSPrimitiveValue::UnitType::kPixels));
      }
      return result;
    }
    case TransformOperation::kRotateX:
    case TransformOperation::kRotateY:
    case TransformOperation::kRotate3D: {
      const auto& rotate = ToRotateTransformOperation(operation);
      CSSFunctionValue* result = CSSFunctionValue::Create(CSSValueRotate3d);
      result->Append(*CSSPrimitiveValue::Create(
          rotate.X(), CSSPrimitiveValue::UnitType::kNumber));
      result->Append(*CSSPrimitiveValue::Create(
          rotate.Y(), CSSPrimitiveValue::UnitType::kNumber));
      result->Append(*CSSPrimitiveValue::Create(
          rotate.Z(), CSSPrimitiveValue::UnitType::kNumber));
      result->Append(*CSSPrimitiveValue::Create(
          rotate.Angle(), CSSPrimitiveValue::UnitType::kDegrees));
      return result;
    }
    case TransformOperation::kRotate: {
      const auto& rotate = ToRotateTransformOperation(operation);
      CSSFunctionValue* result = CSSFunctionValue::Create(CSSValueRotate);
      result->Append(*CSSPrimitiveValue::Create(
          rotate.Angle(), CSSPrimitiveValue::UnitType::kDegrees));
      return result;
    }
    case TransformOperation::kSkewX: {
      const auto& skew = ToSkewTransformOperation(operation);
      CSSFunctionValue* result = CSSFunctionValue::Create(CSSValueSkewX);
      result->Append(*CSSPrimitiveValue::Create(
          skew.AngleX(), CSSPrimitiveValue::UnitType::kDegrees));
      return result;
    }
    case TransformOperation::kSkewY: {
      const auto& skew = ToSkewTransformOperation(operation);
      CSSFunctionValue* result = CSSFunctionValue::Create(CSSValueSkewY);
      result->Append(*CSSPrimitiveValue::Create(
          skew.AngleY(), CSSPrimitiveValue::UnitType::kDegrees));
      return result;
    }
    case TransformOperation::kSkew: {
      const auto& skew = ToSkewTransformOperation(operation);
      CSSFunctionValue* result = CSSFunctionValue::Create(CSSValueSkew);
      result->Append(*CSSPrimitiveValue::Create(
          skew.AngleX(), CSSPrimitiveValue::UnitType::kDegrees));
      result->Append(*CSSPrimitiveValue::Create(
          skew.AngleY(), CSSPrimitiveValue::UnitType::kDegrees));
      return result;
    }
    case TransformOperation::kPerspective: {
      const auto& perspective = ToPerspectiveTransformOperation(operation);
      CSSFunctionValue* result = CSSFunctionValue::Create(CSSValuePerspective);
      result->Append(*CSSPrimitiveValue::Create(
          perspective.Perspective(), CSSPrimitiveValue::UnitType::kPixels));
      return result;
    }
    case TransformOperation::kMatrix: {
      const auto& matrix = ToMatrixTransformOperation(operation).Matrix();
      CSSFunctionValue* result = CSSFunctionValue::Create(CSSValueMatrix);
      double values[6] = {matrix.A(), matrix.B(), matrix.C(),
                          matrix.D(), matrix.E(), matrix.F()};
      for (double value : values) {
        result->Append(*CSSPrimitiveValue::Create(
            value, CSSPrimitiveValue::UnitType::kNumber));
      }
      return result;
    }
    case TransformOperation::kMatrix3D: {
      const auto& matrix = ToMatrix3DTransformOperation(operation).Matrix();
      CSSFunctionValue* result = CSSFunctionValue::Create(CSSValueMatrix3d);
      double values[16] = {
          matrix.M11(), matrix.M12(), matrix.M13(), matrix.M14(),
          matrix.M21(), matrix.M22(), matrix.M23(), matrix.M24(),
          matrix.M31(), matrix.M32(), matrix.M33(), matrix.M34(),
          matrix.M41(), matrix.M42(), matrix.M43(), matrix.M44()};
      for (double value : values) {
        result->Append(*CSSPrimitiveValue::Create(
            value, CSSPrimitiveValue::UnitType::kNumber));
      }
      return result;
    }
    case TransformOperation::kInterpolated:
      // TODO(816803): The computed value in this case is not fully spec'd
      // See https://github.com/w3c/css-houdini-drafts/issues/425
      return CSSIdentifierValue::Create(CSSValueNone);
    default:
      // The remaining operations are unsupported.
      NOTREACHED();
      return CSSIdentifierValue::Create(CSSValueNone);
  }
}

const CSSValue* ComputedTransform(const ComputedStyle& style) {
  if (style.Transform().Operations().size() == 0)
    return CSSIdentifierValue::Create(CSSValueNone);

  CSSValueList* components = CSSValueList::CreateSpaceSeparated();
  for (const auto& operation : style.Transform().Operations()) {
    components->Append(
        *ComputedTransformComponent(*operation, style.EffectiveZoom()));
  }
  return components;
}

}  // namespace

unsigned int ComputedStylePropertyMap::size() {
  const ComputedStyle* style = UpdateStyle();
  if (!style)
    return 0;

  const auto& variables = ComputedStyleCSSValueMapping::GetVariables(*style);
  return CSSComputedStyleDeclaration::ComputableProperties().size() +
         (variables ? variables->size() : 0);
}

bool ComputedStylePropertyMap::ComparePropertyNames(const String& a,
                                                    const String& b) {
  if (a.StartsWith("--"))
    return b.StartsWith("--") && WTF::CodePointCompareLessThan(a, b);
  if (a.StartsWith("-")) {
    return b.StartsWith("--") ||
           (b.StartsWith("-") && WTF::CodePointCompareLessThan(a, b));
  }
  return b.StartsWith("-") || WTF::CodePointCompareLessThan(a, b);
}

Node* ComputedStylePropertyMap::StyledNode() const {
  DCHECK(node_);
  if (!pseudo_id_)
    return node_;
  if (node_->IsElementNode()) {
    if (PseudoElement* element =
            (ToElement(node_))->GetPseudoElement(pseudo_id_)) {
      return element;
    }
  }
  return nullptr;
}

const ComputedStyle* ComputedStylePropertyMap::UpdateStyle() {
  Node* node = StyledNode();
  if (!node || !node->InActiveDocument())
    return nullptr;

  // Update style before getting the value for the property
  // This could cause the node to be blown away. This code is copied from
  // CSSComputedStyleDeclaration::GetPropertyCSSValue.
  node->GetDocument().UpdateStyleAndLayoutTreeForNode(node);
  node = StyledNode();
  if (!node)
    return nullptr;
  // This is copied from CSSComputedStyleDeclaration::computeComputedStyle().
  // PseudoIdNone must be used if node() is a PseudoElement.
  const ComputedStyle* style = node->EnsureComputedStyle(
      node->IsPseudoElement() ? kPseudoIdNone : pseudo_id_);
  node = StyledNode();
  if (!node || !node->InActiveDocument() || !style)
    return nullptr;
  return style;
}

const CSSValue* ComputedStylePropertyMap::GetProperty(
    CSSPropertyID property_id) {
  const ComputedStyle* style = UpdateStyle();
  if (!style)
    return nullptr;

  // Special cases for properties where CSSProperty::CSSValueFromComputedStyle
  // doesn't return the correct computed value
  switch (property_id) {
    case CSSPropertyTransform:
      return ComputedTransform(*style);
    default:
      return CSSProperty::Get(property_id)
          .CSSValueFromComputedStyle(*style, nullptr /* layout_object */,
                                     StyledNode(), false);
  }
}

const CSSValue* ComputedStylePropertyMap::GetCustomProperty(
    AtomicString property_name) {
  const ComputedStyle* style = UpdateStyle();
  if (!style)
    return nullptr;
  return ComputedStyleCSSValueMapping::Get(
      property_name, *style, node_->GetDocument().GetPropertyRegistry());
}

void ComputedStylePropertyMap::ForEachProperty(
    const IterationCallback& callback) {
  const ComputedStyle* style = UpdateStyle();
  if (!style)
    return;

  // Have to sort by all properties by code point, so we have to store
  // them in a buffer first.
  HeapVector<std::pair<AtomicString, Member<const CSSValue>>> values;
  for (const CSSProperty* property :
       CSSComputedStyleDeclaration::ComputableProperties()) {
    DCHECK(property);
    DCHECK(!property->IDEquals(CSSPropertyVariable));
    const CSSValue* value = property->CSSValueFromComputedStyle(
        *style, nullptr /* layout_object */, StyledNode(), false);
    if (value)
      values.emplace_back(property->GetPropertyNameAtomicString(), value);
  }

  const auto& variables = ComputedStyleCSSValueMapping::GetVariables(*style);
  if (variables) {
    for (const auto& name_value : *variables) {
      if (name_value.value) {
        values.emplace_back(name_value.key,
                            CSSCustomPropertyDeclaration::Create(
                                name_value.key, name_value.value));
      }
    }
  }

  std::sort(values.begin(), values.end(), [](const auto& a, const auto& b) {
    return ComparePropertyNames(a.first, b.first);
  });

  for (const auto& value : values)
    callback(value.first, *value.second);
}

String ComputedStylePropertyMap::SerializationForShorthand(
    const CSSProperty& property) {
  DCHECK(property.IsShorthand());
  const ComputedStyle* style = UpdateStyle();
  if (!style) {
    NOTREACHED();
    return "";
  }

  if (const CSSValue* value = property.CSSValueFromComputedStyle(
          *style, nullptr /* layout_object */, StyledNode(), false)) {
    return value->CssText();
  }

  NOTREACHED();
  return "";
}

}  // namespace blink
