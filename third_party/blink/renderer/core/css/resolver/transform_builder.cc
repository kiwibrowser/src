/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 2004-2005 Allan Sandfeld Jensen (kde@carewolf.com)
 * Copyright (C) 2006, 2007 Nicholas Shanks (webkit@nickshanks.com)
 * Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013 Apple Inc.
 * All rights reserved.
 * Copyright (C) 2007 Alexey Proskuryakov <ap@webkit.org>
 * Copyright (C) 2007, 2008 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved.
 * (http://www.torchmobile.com/)
 * Copyright (c) 2011, Code Aurora Forum. All rights reserved.
 * Copyright (C) Research In Motion Limited 2011. All rights reserved.
 * Copyright (C) 2012 Google Inc. All rights reserved.
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

#include "third_party/blink/renderer/core/css/resolver/transform_builder.h"

#include "third_party/blink/renderer/core/css/css_function_value.h"
#include "third_party/blink/renderer/core/css/css_primitive_value_mappings.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/transforms/matrix_3d_transform_operation.h"
#include "third_party/blink/renderer/platform/transforms/matrix_transform_operation.h"
#include "third_party/blink/renderer/platform/transforms/perspective_transform_operation.h"
#include "third_party/blink/renderer/platform/transforms/rotate_transform_operation.h"
#include "third_party/blink/renderer/platform/transforms/scale_transform_operation.h"
#include "third_party/blink/renderer/platform/transforms/skew_transform_operation.h"
#include "third_party/blink/renderer/platform/transforms/transformation_matrix.h"
#include "third_party/blink/renderer/platform/transforms/translate_transform_operation.h"

namespace blink {

static Length ConvertToFloatLength(
    const CSSPrimitiveValue& primitive_value,
    const CSSToLengthConversionData& conversion_data) {
  return primitive_value.ConvertToLength(conversion_data);
}

static TransformOperation::OperationType GetTransformOperationType(
    CSSValueID type) {
  switch (type) {
    default:
      NOTREACHED();
      FALLTHROUGH;
    case CSSValueScale:
      return TransformOperation::kScale;
    case CSSValueScaleX:
      return TransformOperation::kScaleX;
    case CSSValueScaleY:
      return TransformOperation::kScaleY;
    case CSSValueScaleZ:
      return TransformOperation::kScaleZ;
    case CSSValueScale3d:
      return TransformOperation::kScale3D;
    case CSSValueTranslate:
      return TransformOperation::kTranslate;
    case CSSValueTranslateX:
      return TransformOperation::kTranslateX;
    case CSSValueTranslateY:
      return TransformOperation::kTranslateY;
    case CSSValueTranslateZ:
      return TransformOperation::kTranslateZ;
    case CSSValueTranslate3d:
      return TransformOperation::kTranslate3D;
    case CSSValueRotate:
      return TransformOperation::kRotate;
    case CSSValueRotateX:
      return TransformOperation::kRotateX;
    case CSSValueRotateY:
      return TransformOperation::kRotateY;
    case CSSValueRotateZ:
      return TransformOperation::kRotateZ;
    case CSSValueRotate3d:
      return TransformOperation::kRotate3D;
    case CSSValueSkew:
      return TransformOperation::kSkew;
    case CSSValueSkewX:
      return TransformOperation::kSkewX;
    case CSSValueSkewY:
      return TransformOperation::kSkewY;
    case CSSValueMatrix:
      return TransformOperation::kMatrix;
    case CSSValueMatrix3d:
      return TransformOperation::kMatrix3D;
    case CSSValuePerspective:
      return TransformOperation::kPerspective;
  }
}

bool TransformBuilder::HasRelativeLengths(const CSSValueList& value_list) {
  for (auto& value : value_list) {
    const CSSFunctionValue* transform_value = ToCSSFunctionValue(value.Get());

    for (const CSSValue* item : *transform_value) {
      const CSSPrimitiveValue& primitive_value = ToCSSPrimitiveValue(*item);

      if (primitive_value.IsCalculated()) {
        CSSCalcValue* css_calc_value = primitive_value.CssCalcValue();
        CSSPrimitiveValue::UnitType resolved_type =
            css_calc_value->ExpressionNode()->TypeWithCalcResolved();
        if (CSSPrimitiveValue::IsRelativeUnit(resolved_type) ||
            resolved_type == CSSPrimitiveValue::UnitType::kUnknown) {
          return true;
        }
      }

      if (CSSPrimitiveValue::IsRelativeUnit(
              primitive_value.TypeWithCalcResolved())) {
        return true;
      }
    }
  }
  return false;
}

TransformOperations TransformBuilder::CreateTransformOperations(
    const CSSValue& in_value,
    const CSSToLengthConversionData& conversion_data) {
  TransformOperations operations;
  if (!in_value.IsValueList()) {
    DCHECK_EQ(ToCSSIdentifierValue(in_value).GetValueID(), CSSValueNone);
    return operations;
  }

  float zoom_factor = conversion_data.Zoom();
  for (auto& value : ToCSSValueList(in_value)) {
    const CSSFunctionValue* transform_value = ToCSSFunctionValue(value.Get());
    TransformOperation::OperationType transform_type =
        GetTransformOperationType(transform_value->FunctionType());

    const CSSPrimitiveValue& first_value =
        ToCSSPrimitiveValue(transform_value->Item(0));

    switch (transform_type) {
      case TransformOperation::kScale:
      case TransformOperation::kScaleX:
      case TransformOperation::kScaleY: {
        double sx = 1.0;
        double sy = 1.0;
        if (transform_type == TransformOperation::kScaleY) {
          sy = first_value.GetDoubleValue();
        } else {
          sx = first_value.GetDoubleValue();
          if (transform_type != TransformOperation::kScaleX) {
            if (transform_value->length() > 1) {
              const CSSPrimitiveValue& second_value =
                  ToCSSPrimitiveValue(transform_value->Item(1));
              sy = second_value.GetDoubleValue();
            } else {
              sy = sx;
            }
          }
        }
        operations.Operations().push_back(
            ScaleTransformOperation::Create(sx, sy, 1.0, transform_type));
        break;
      }
      case TransformOperation::kScaleZ:
      case TransformOperation::kScale3D: {
        double sx = 1.0;
        double sy = 1.0;
        double sz = 1.0;
        if (transform_type == TransformOperation::kScaleZ) {
          sz = first_value.GetDoubleValue();
        } else {
          sx = first_value.GetDoubleValue();
          sy = ToCSSPrimitiveValue(transform_value->Item(1)).GetDoubleValue();
          sz = ToCSSPrimitiveValue(transform_value->Item(2)).GetDoubleValue();
        }
        operations.Operations().push_back(
            ScaleTransformOperation::Create(sx, sy, sz, transform_type));
        break;
      }
      case TransformOperation::kTranslate:
      case TransformOperation::kTranslateX:
      case TransformOperation::kTranslateY: {
        Length tx = Length(0, kFixed);
        Length ty = Length(0, kFixed);
        if (transform_type == TransformOperation::kTranslateY)
          ty = ConvertToFloatLength(first_value, conversion_data);
        else {
          tx = ConvertToFloatLength(first_value, conversion_data);
          if (transform_type != TransformOperation::kTranslateX) {
            if (transform_value->length() > 1) {
              const CSSPrimitiveValue& second_value =
                  ToCSSPrimitiveValue(transform_value->Item(1));
              ty = ConvertToFloatLength(second_value, conversion_data);
            }
          }
        }

        operations.Operations().push_back(
            TranslateTransformOperation::Create(tx, ty, 0, transform_type));
        break;
      }
      case TransformOperation::kTranslateZ:
      case TransformOperation::kTranslate3D: {
        Length tx = Length(0, kFixed);
        Length ty = Length(0, kFixed);
        double tz = 0;
        if (transform_type == TransformOperation::kTranslateZ) {
          tz = first_value.ComputeLength<double>(conversion_data);
        } else {
          tx = ConvertToFloatLength(first_value, conversion_data);
          ty = ConvertToFloatLength(
              ToCSSPrimitiveValue(transform_value->Item(1)), conversion_data);
          tz = ToCSSPrimitiveValue(transform_value->Item(2))
                   .ComputeLength<double>(conversion_data);
        }

        operations.Operations().push_back(
            TranslateTransformOperation::Create(tx, ty, tz, transform_type));
        break;
      }
      case TransformOperation::kRotateX:
      case TransformOperation::kRotateY:
      case TransformOperation::kRotateZ: {
        double angle = first_value.ComputeDegrees();
        if (transform_value->length() == 1) {
          double x = transform_type == TransformOperation::kRotateX;
          double y = transform_type == TransformOperation::kRotateY;
          double z = transform_type == TransformOperation::kRotateZ;
          operations.Operations().push_back(
              RotateTransformOperation::Create(x, y, z, angle, transform_type));
        } else {
          // For SVG 'transform' attributes we generate 3-argument rotate()
          // functions.
          DCHECK_EQ(transform_value->length(), 3u);
          const CSSPrimitiveValue& second_value =
              ToCSSPrimitiveValue(transform_value->Item(1));
          const CSSPrimitiveValue& third_value =
              ToCSSPrimitiveValue(transform_value->Item(2));
          operations.Operations().push_back(
              RotateAroundOriginTransformOperation::Create(
                  angle, second_value.ComputeLength<double>(conversion_data),
                  third_value.ComputeLength<double>(conversion_data)));
        }
        break;
      }
      case TransformOperation::kRotate3D: {
        const CSSPrimitiveValue& second_value =
            ToCSSPrimitiveValue(transform_value->Item(1));
        const CSSPrimitiveValue& third_value =
            ToCSSPrimitiveValue(transform_value->Item(2));
        const CSSPrimitiveValue& fourth_value =
            ToCSSPrimitiveValue(transform_value->Item(3));
        double x = first_value.GetDoubleValue();
        double y = second_value.GetDoubleValue();
        double z = third_value.GetDoubleValue();
        double angle = fourth_value.ComputeDegrees();
        operations.Operations().push_back(
            RotateTransformOperation::Create(x, y, z, angle, transform_type));
        break;
      }
      case TransformOperation::kSkew:
      case TransformOperation::kSkewX:
      case TransformOperation::kSkewY: {
        double angle_x = 0;
        double angle_y = 0;
        double angle = first_value.ComputeDegrees();
        if (transform_type == TransformOperation::kSkewY)
          angle_y = angle;
        else {
          angle_x = angle;
          if (transform_type == TransformOperation::kSkew) {
            if (transform_value->length() > 1) {
              const CSSPrimitiveValue& second_value =
                  ToCSSPrimitiveValue(transform_value->Item(1));
              angle_y = second_value.ComputeDegrees();
            }
          }
        }
        operations.Operations().push_back(
            SkewTransformOperation::Create(angle_x, angle_y, transform_type));
        break;
      }
      case TransformOperation::kMatrix: {
        double a = first_value.GetDoubleValue();
        double b =
            ToCSSPrimitiveValue(transform_value->Item(1)).GetDoubleValue();
        double c =
            ToCSSPrimitiveValue(transform_value->Item(2)).GetDoubleValue();
        double d =
            ToCSSPrimitiveValue(transform_value->Item(3)).GetDoubleValue();
        double e =
            zoom_factor *
            ToCSSPrimitiveValue(transform_value->Item(4)).GetDoubleValue();
        double f =
            zoom_factor *
            ToCSSPrimitiveValue(transform_value->Item(5)).GetDoubleValue();
        operations.Operations().push_back(
            MatrixTransformOperation::Create(a, b, c, d, e, f));
        break;
      }
      case TransformOperation::kMatrix3D: {
        TransformationMatrix matrix(
            ToCSSPrimitiveValue(transform_value->Item(0)).GetDoubleValue(),
            ToCSSPrimitiveValue(transform_value->Item(1)).GetDoubleValue(),
            ToCSSPrimitiveValue(transform_value->Item(2)).GetDoubleValue(),
            ToCSSPrimitiveValue(transform_value->Item(3)).GetDoubleValue(),
            ToCSSPrimitiveValue(transform_value->Item(4)).GetDoubleValue(),
            ToCSSPrimitiveValue(transform_value->Item(5)).GetDoubleValue(),
            ToCSSPrimitiveValue(transform_value->Item(6)).GetDoubleValue(),
            ToCSSPrimitiveValue(transform_value->Item(7)).GetDoubleValue(),
            ToCSSPrimitiveValue(transform_value->Item(8)).GetDoubleValue(),
            ToCSSPrimitiveValue(transform_value->Item(9)).GetDoubleValue(),
            ToCSSPrimitiveValue(transform_value->Item(10)).GetDoubleValue(),
            ToCSSPrimitiveValue(transform_value->Item(11)).GetDoubleValue(),
            ToCSSPrimitiveValue(transform_value->Item(12)).GetDoubleValue(),
            ToCSSPrimitiveValue(transform_value->Item(13)).GetDoubleValue(),
            ToCSSPrimitiveValue(transform_value->Item(14)).GetDoubleValue(),
            ToCSSPrimitiveValue(transform_value->Item(15)).GetDoubleValue());
        matrix.Zoom(zoom_factor);
        operations.Operations().push_back(
            Matrix3DTransformOperation::Create(matrix));
        break;
      }
      case TransformOperation::kPerspective: {
        double p = first_value.ComputeLength<double>(conversion_data);
        DCHECK_GE(p, 0);
        operations.Operations().push_back(
            PerspectiveTransformOperation::Create(p));
        break;
      }
      default:
        NOTREACHED();
        break;
    }
  }
  return operations;
}

}  // namespace blink
