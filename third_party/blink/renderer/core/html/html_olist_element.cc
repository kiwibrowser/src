/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
 *
 */

#include "third_party/blink/renderer/core/html/html_olist_element.h"

#include "third_party/blink/renderer/core/css_property_names.h"
#include "third_party/blink/renderer/core/css_value_keywords.h"
#include "third_party/blink/renderer/core/frame/use_counter.h"
#include "third_party/blink/renderer/core/html/list_item_ordinal.h"
#include "third_party/blink/renderer/core/html/parser/html_parser_idioms.h"
#include "third_party/blink/renderer/core/html_names.h"
#include "third_party/blink/renderer/core/layout/layout_list_item.h"

namespace blink {

using namespace HTMLNames;

inline HTMLOListElement::HTMLOListElement(Document& document)
    : HTMLElement(olTag, document),
      start_(0xBADBEEF),
      item_count_(0),
      has_explicit_start_(false),
      is_reversed_(false),
      should_recalculate_item_count_(false) {}

DEFINE_NODE_FACTORY(HTMLOListElement)

bool HTMLOListElement::IsPresentationAttribute(
    const QualifiedName& name) const {
  if (name == typeAttr)
    return true;
  return HTMLElement::IsPresentationAttribute(name);
}

void HTMLOListElement::CollectStyleForPresentationAttribute(
    const QualifiedName& name,
    const AtomicString& value,
    MutableCSSPropertyValueSet* style) {
  if (name == typeAttr) {
    if (value == "a")
      AddPropertyToPresentationAttributeStyle(style, CSSPropertyListStyleType,
                                              CSSValueLowerAlpha);
    else if (value == "A")
      AddPropertyToPresentationAttributeStyle(style, CSSPropertyListStyleType,
                                              CSSValueUpperAlpha);
    else if (value == "i")
      AddPropertyToPresentationAttributeStyle(style, CSSPropertyListStyleType,
                                              CSSValueLowerRoman);
    else if (value == "I")
      AddPropertyToPresentationAttributeStyle(style, CSSPropertyListStyleType,
                                              CSSValueUpperRoman);
    else if (value == "1")
      AddPropertyToPresentationAttributeStyle(style, CSSPropertyListStyleType,
                                              CSSValueDecimal);
  } else {
    HTMLElement::CollectStyleForPresentationAttribute(name, value, style);
  }
}

void HTMLOListElement::ParseAttribute(
    const AttributeModificationParams& params) {
  if (params.name == startAttr) {
    int old_start = StartConsideringItemCount();
    int parsed_start = 0;
    bool can_parse = ParseHTMLInteger(params.new_value, parsed_start);
    has_explicit_start_ = can_parse;
    start_ = can_parse ? parsed_start : 0xBADBEEF;
    if (old_start == StartConsideringItemCount())
      return;
    UpdateItemValues();
  } else if (params.name == reversedAttr) {
    bool reversed = !params.new_value.IsNull();
    if (reversed == is_reversed_)
      return;
    is_reversed_ = reversed;
    UpdateItemValues();
  } else {
    HTMLElement::ParseAttribute(params);
  }
}

void HTMLOListElement::setStart(int start) {
  SetIntegralAttribute(startAttr, start);
}

void HTMLOListElement::UpdateItemValues() {
  if (!GetLayoutObject())
    return;
  UpdateDistributionForFlatTreeTraversal();
  ListItemOrdinal::InvalidateAllItemsForOrderedList(this);
}

void HTMLOListElement::RecalculateItemCount() {
  item_count_ = ListItemOrdinal::ItemCountForOrderedList(this);
  should_recalculate_item_count_ = false;
}

}  // namespace blink
