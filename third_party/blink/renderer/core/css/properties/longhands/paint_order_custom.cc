// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/properties/longhands/paint_order.h"

#include "third_party/blink/renderer/core/css/css_value_list.h"
#include "third_party/blink/renderer/core/css/parser/css_property_parser_helpers.h"
#include "third_party/blink/renderer/core/css/properties/computed_style_utils.h"
#include "third_party/blink/renderer/core/style/computed_style.h"

namespace blink {
namespace CSSLonghand {

const CSSValue* PaintOrder::ParseSingleValue(
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    const CSSParserLocalContext&) const {
  if (range.Peek().Id() == CSSValueNormal)
    return CSSPropertyParserHelpers::ConsumeIdent(range);

  Vector<CSSValueID, 3> paint_type_list;
  CSSIdentifierValue* fill = nullptr;
  CSSIdentifierValue* stroke = nullptr;
  CSSIdentifierValue* markers = nullptr;
  do {
    CSSValueID id = range.Peek().Id();
    if (id == CSSValueFill && !fill)
      fill = CSSPropertyParserHelpers::ConsumeIdent(range);
    else if (id == CSSValueStroke && !stroke)
      stroke = CSSPropertyParserHelpers::ConsumeIdent(range);
    else if (id == CSSValueMarkers && !markers)
      markers = CSSPropertyParserHelpers::ConsumeIdent(range);
    else
      return nullptr;
    paint_type_list.push_back(id);
  } while (!range.AtEnd());

  // After parsing we serialize the paint-order list. Since it is not possible
  // to pop a last list items from CSSValueList without bigger cost, we create
  // the list after parsing.
  CSSValueID first_paint_order_type = paint_type_list.at(0);
  CSSValueList* paint_order_list = CSSValueList::CreateSpaceSeparated();
  switch (first_paint_order_type) {
    case CSSValueFill:
    case CSSValueStroke:
      paint_order_list->Append(
          first_paint_order_type == CSSValueFill ? *fill : *stroke);
      if (paint_type_list.size() > 1) {
        if (paint_type_list.at(1) == CSSValueMarkers)
          paint_order_list->Append(*markers);
      }
      break;
    case CSSValueMarkers:
      paint_order_list->Append(*markers);
      if (paint_type_list.size() > 1) {
        if (paint_type_list.at(1) == CSSValueStroke)
          paint_order_list->Append(*stroke);
      }
      break;
    default:
      NOTREACHED();
  }

  return paint_order_list;
}

const CSSValue* PaintOrder::CSSValueFromComputedStyleInternal(
    const ComputedStyle&,
    const SVGComputedStyle& svg_style,
    const LayoutObject*,
    Node* styled_node,
    bool allow_visited_style) const {
  return ComputedStyleUtils::PaintOrderToCSSValueList(svg_style);
}

}  // namespace CSSLonghand
}  // namespace blink
