// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/properties/longhands/touch_action.h"

#include "third_party/blink/renderer/core/css/css_value_list.h"
#include "third_party/blink/renderer/core/css/parser/css_property_parser_helpers.h"
#include "third_party/blink/renderer/core/css/properties/computed_style_utils.h"
#include "third_party/blink/renderer/core/style/computed_style.h"

namespace blink {
namespace {

static bool ConsumePan(CSSParserTokenRange& range,
                       CSSValue*& pan_x,
                       CSSValue*& pan_y,
                       CSSValue*& pinch_zoom) {
  CSSValueID id = range.Peek().Id();
  if ((id == CSSValuePanX || id == CSSValuePanRight || id == CSSValuePanLeft) &&
      !pan_x) {
    pan_x = CSSPropertyParserHelpers::ConsumeIdent(range);
  } else if ((id == CSSValuePanY || id == CSSValuePanDown ||
              id == CSSValuePanUp) &&
             !pan_y) {
    pan_y = CSSPropertyParserHelpers::ConsumeIdent(range);
  } else if (id == CSSValuePinchZoom && !pinch_zoom) {
    pinch_zoom = CSSPropertyParserHelpers::ConsumeIdent(range);
  } else {
    return false;
  }
  return true;
}

}  // namespace
namespace CSSLonghand {

const CSSValue* TouchAction::ParseSingleValue(
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    const CSSParserLocalContext&) const {
  CSSValueList* list = CSSValueList::CreateSpaceSeparated();
  CSSValueID id = range.Peek().Id();
  if (id == CSSValueAuto || id == CSSValueNone || id == CSSValueManipulation) {
    list->Append(*CSSPropertyParserHelpers::ConsumeIdent(range));
    return list;
  }

  CSSValue* pan_x = nullptr;
  CSSValue* pan_y = nullptr;
  CSSValue* pinch_zoom = nullptr;
  if (!ConsumePan(range, pan_x, pan_y, pinch_zoom))
    return nullptr;
  if (!range.AtEnd() && !ConsumePan(range, pan_x, pan_y, pinch_zoom))
    return nullptr;
  if (!range.AtEnd() && !ConsumePan(range, pan_x, pan_y, pinch_zoom))
    return nullptr;

  if (pan_x)
    list->Append(*pan_x);
  if (pan_y)
    list->Append(*pan_y);
  if (pinch_zoom)
    list->Append(*pinch_zoom);
  return list;
}

const CSSValue* TouchAction::CSSValueFromComputedStyleInternal(
    const ComputedStyle& style,
    const SVGComputedStyle&,
    const LayoutObject*,
    Node* styled_node,
    bool allow_visited_style) const {
  return ComputedStyleUtils::TouchActionFlagsToCSSValue(style.GetTouchAction());
}

}  // namespace CSSLonghand
}  // namespace blink
