// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSS_VALUE_ID_MAPPINGS_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSS_VALUE_ID_MAPPINGS_H_

#include "third_party/blink/renderer/core/css_value_id_mappings_generated.h"

namespace blink {

template <class T>
T CssValueIDToPlatformEnum(CSSValueID v) {
  // By default, we use the generated mappings. For special cases, we
  // specialize.
  return detail::cssValueIDToPlatformEnumGenerated<T>(v);
}

template <class T>
inline CSSValueID PlatformEnumToCSSValueID(T v) {
  // By default, we use the generated mappings. For special cases, we overload.
  return detail::platformEnumToCSSValueIDGenerated(v);
}

template <>
inline UnicodeBidi CssValueIDToPlatformEnum(CSSValueID v) {
  if (v == CSSValueWebkitIsolate)
    return UnicodeBidi::kIsolate;
  if (v == CSSValueWebkitIsolateOverride)
    return UnicodeBidi::kIsolateOverride;
  if (v == CSSValueWebkitPlaintext)
    return UnicodeBidi::kPlaintext;
  return detail::cssValueIDToPlatformEnumGenerated<UnicodeBidi>(v);
}

template <>
inline EBoxOrient CssValueIDToPlatformEnum(CSSValueID v) {
  if (v == CSSValueInlineAxis)
    return EBoxOrient::kHorizontal;
  if (v == CSSValueBlockAxis)
    return EBoxOrient::kVertical;

  return detail::cssValueIDToPlatformEnumGenerated<EBoxOrient>(v);
}

template <>
inline ETextCombine CssValueIDToPlatformEnum(CSSValueID v) {
  if (v == CSSValueHorizontal)  // -webkit-text-combine
    return ETextCombine::kAll;
  return detail::cssValueIDToPlatformEnumGenerated<ETextCombine>(v);
}

template <>
inline ETextAlign CssValueIDToPlatformEnum(CSSValueID v) {
  if (v == CSSValueWebkitAuto)  // Legacy -webkit-auto. Eqiuvalent to start.
    return ETextAlign::kStart;
  if (v == CSSValueInternalCenter)
    return ETextAlign::kCenter;
  return detail::cssValueIDToPlatformEnumGenerated<ETextAlign>(v);
}

template <>
inline ETextOrientation CssValueIDToPlatformEnum(CSSValueID v) {
  if (v == CSSValueSidewaysRight)  // Legacy -webkit-auto. Eqiuvalent to start.
    return ETextOrientation::kSideways;
  if (v == CSSValueVerticalRight)
    return ETextOrientation::kMixed;
  return detail::cssValueIDToPlatformEnumGenerated<ETextOrientation>(v);
}

template <>
inline EResize CssValueIDToPlatformEnum(CSSValueID v) {
  if (v == CSSValueAuto) {
    // Depends on settings, thus should be handled by the caller.
    NOTREACHED();
    return EResize::kNone;
  }
  return detail::cssValueIDToPlatformEnumGenerated<EResize>(v);
}

template <>
inline WritingMode CssValueIDToPlatformEnum(CSSValueID v) {
  switch (v) {
    case CSSValueHorizontalTb:
    case CSSValueLr:
    case CSSValueLrTb:
    case CSSValueRl:
    case CSSValueRlTb:
      return WritingMode::kHorizontalTb;
    case CSSValueVerticalRl:
    case CSSValueTb:
    case CSSValueTbRl:
      return WritingMode::kVerticalRl;
    case CSSValueVerticalLr:
      return WritingMode::kVerticalLr;
    default:
      break;
  }

  NOTREACHED();
  return WritingMode::kHorizontalTb;
}

template <>
inline ECursor CssValueIDToPlatformEnum(CSSValueID v) {
  if (v == CSSValueWebkitZoomIn)
    return ECursor::kZoomIn;
  if (v == CSSValueWebkitZoomOut)
    return ECursor::kZoomOut;
  if (v == CSSValueWebkitGrab)
    return ECursor::kGrab;
  if (v == CSSValueWebkitGrabbing)
    return ECursor::kGrabbing;
  return detail::cssValueIDToPlatformEnumGenerated<ECursor>(v);
}

template <>
inline EDisplay CssValueIDToPlatformEnum(CSSValueID v) {
  if (v == CSSValueNone)
    return EDisplay::kNone;
  if (v == CSSValueInline)
    return EDisplay::kInline;
  if (v == CSSValueBlock)
    return EDisplay::kBlock;
  if (v == CSSValueFlowRoot)
    return EDisplay::kFlowRoot;
  if (v == CSSValueListItem)
    return EDisplay::kListItem;
  if (v == CSSValueInlineBlock)
    return EDisplay::kInlineBlock;
  if (v == CSSValueTable)
    return EDisplay::kTable;
  if (v == CSSValueInlineTable)
    return EDisplay::kInlineTable;
  if (v == CSSValueTableRowGroup)
    return EDisplay::kTableRowGroup;
  if (v == CSSValueTableHeaderGroup)
    return EDisplay::kTableHeaderGroup;
  if (v == CSSValueTableFooterGroup)
    return EDisplay::kTableFooterGroup;
  if (v == CSSValueTableRow)
    return EDisplay::kTableRow;
  if (v == CSSValueTableColumnGroup)
    return EDisplay::kTableColumnGroup;
  if (v == CSSValueTableColumn)
    return EDisplay::kTableColumn;
  if (v == CSSValueTableCell)
    return EDisplay::kTableCell;
  if (v == CSSValueTableCaption)
    return EDisplay::kTableCaption;
  if (v == CSSValueWebkitBox)
    return EDisplay::kWebkitBox;
  if (v == CSSValueWebkitInlineBox)
    return EDisplay::kWebkitInlineBox;
  if (v == CSSValueFlex)
    return EDisplay::kFlex;
  if (v == CSSValueInlineFlex)
    return EDisplay::kInlineFlex;
  if (v == CSSValueGrid)
    return EDisplay::kGrid;
  if (v == CSSValueInlineGrid)
    return EDisplay::kInlineGrid;
  if (v == CSSValueContents)
    return EDisplay::kContents;
  if (v == CSSValueWebkitFlex)
    return EDisplay::kFlex;
  if (v == CSSValueWebkitInlineFlex)
    return EDisplay::kInlineFlex;

  NOTREACHED();
  return EDisplay::kInline;
}

template <>
inline EUserSelect CssValueIDToPlatformEnum(CSSValueID v) {
  if (v == CSSValueAuto)
    return EUserSelect::kAuto;
  return detail::cssValueIDToPlatformEnumGenerated<EUserSelect>(v);
}

template <>
inline CSSValueID PlatformEnumToCSSValueID(EDisplay v) {
  if (v == EDisplay::kNone)
    return CSSValueNone;
  if (v == EDisplay::kInline)
    return CSSValueInline;
  if (v == EDisplay::kBlock)
    return CSSValueBlock;
  if (v == EDisplay::kFlowRoot)
    return CSSValueFlowRoot;
  if (v == EDisplay::kListItem)
    return CSSValueListItem;
  if (v == EDisplay::kInlineBlock)
    return CSSValueInlineBlock;
  if (v == EDisplay::kTable)
    return CSSValueTable;
  if (v == EDisplay::kInlineTable)
    return CSSValueInlineTable;
  if (v == EDisplay::kTableRowGroup)
    return CSSValueTableRowGroup;
  if (v == EDisplay::kTableHeaderGroup)
    return CSSValueTableHeaderGroup;
  if (v == EDisplay::kTableFooterGroup)
    return CSSValueTableFooterGroup;
  if (v == EDisplay::kTableRow)
    return CSSValueTableRow;
  if (v == EDisplay::kTableColumnGroup)
    return CSSValueTableColumnGroup;
  if (v == EDisplay::kTableColumn)
    return CSSValueTableColumn;
  if (v == EDisplay::kTableCell)
    return CSSValueTableCell;
  if (v == EDisplay::kTableCaption)
    return CSSValueTableCaption;
  if (v == EDisplay::kWebkitBox)
    return CSSValueWebkitBox;
  if (v == EDisplay::kWebkitInlineBox)
    return CSSValueWebkitInlineBox;
  if (v == EDisplay::kFlex)
    return CSSValueFlex;
  if (v == EDisplay::kInlineFlex)
    return CSSValueInlineFlex;
  if (v == EDisplay::kGrid)
    return CSSValueGrid;
  if (v == EDisplay::kInlineGrid)
    return CSSValueInlineGrid;
  if (v == EDisplay::kContents)
    return CSSValueContents;

  NOTREACHED();
  return CSSValueInline;
}

}  // namespace blink

#endif
