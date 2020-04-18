/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2006, 2007, 2010 Apple Inc. All rights reserved.
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

#include "third_party/blink/renderer/core/html/html_li_element.h"

#include "third_party/blink/renderer/core/css_property_names.h"
#include "third_party/blink/renderer/core/css_value_keywords.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/layout_tree_builder_traversal.h"
#include "third_party/blink/renderer/core/html/list_item_ordinal.h"
#include "third_party/blink/renderer/core/html/parser/html_parser_idioms.h"
#include "third_party/blink/renderer/core/html_names.h"

namespace blink {

using namespace HTMLNames;

inline HTMLLIElement::HTMLLIElement(Document& document)
    : HTMLElement(liTag, document) {}

DEFINE_NODE_FACTORY(HTMLLIElement)

bool HTMLLIElement::IsPresentationAttribute(const QualifiedName& name) const {
  if (name == typeAttr)
    return true;
  return HTMLElement::IsPresentationAttribute(name);
}

CSSValueID ListTypeToCSSValueID(const AtomicString& value) {
  if (value == "a")
    return CSSValueLowerAlpha;
  if (value == "A")
    return CSSValueUpperAlpha;
  if (value == "i")
    return CSSValueLowerRoman;
  if (value == "I")
    return CSSValueUpperRoman;
  if (value == "1")
    return CSSValueDecimal;
  if (DeprecatedEqualIgnoringCase(value, "disc"))
    return CSSValueDisc;
  if (DeprecatedEqualIgnoringCase(value, "circle"))
    return CSSValueCircle;
  if (DeprecatedEqualIgnoringCase(value, "square"))
    return CSSValueSquare;
  if (DeprecatedEqualIgnoringCase(value, "none"))
    return CSSValueNone;
  return CSSValueInvalid;
}

void HTMLLIElement::CollectStyleForPresentationAttribute(
    const QualifiedName& name,
    const AtomicString& value,
    MutableCSSPropertyValueSet* style) {
  if (name == typeAttr) {
    CSSValueID type_value = ListTypeToCSSValueID(value);
    if (type_value != CSSValueInvalid)
      AddPropertyToPresentationAttributeStyle(style, CSSPropertyListStyleType,
                                              type_value);
  } else {
    HTMLElement::CollectStyleForPresentationAttribute(name, value, style);
  }
}

void HTMLLIElement::ParseAttribute(const AttributeModificationParams& params) {
  if (params.name == valueAttr) {
    if (ListItemOrdinal* ordinal = ListItemOrdinal::Get(*this))
      ParseValue(params.new_value, ordinal);
  } else {
    HTMLElement::ParseAttribute(params);
  }
}

void HTMLLIElement::AttachLayoutTree(AttachContext& context) {
  HTMLElement::AttachLayoutTree(context);

  if (ListItemOrdinal* ordinal = ListItemOrdinal::Get(*this)) {
    DCHECK(!GetDocument().ChildNeedsDistributionRecalc());

    // Find the enclosing list node.
    Element* list_node = nullptr;
    Element* current = this;
    while (!list_node) {
      current = LayoutTreeBuilderTraversal::ParentElement(*current);
      if (!current)
        break;
      if (IsHTMLUListElement(*current) || IsHTMLOListElement(*current))
        list_node = current;
    }

    // If we are not in a list, tell the layoutObject so it can position us
    // inside.  We don't want to change our style to say "inside" since that
    // would affect nested nodes.
    if (!list_node)
      ordinal->SetNotInList(true);

    ParseValue(FastGetAttribute(valueAttr), ordinal);
  }
}

void HTMLLIElement::ParseValue(const AtomicString& value,
                               ListItemOrdinal* ordinal) {
  DCHECK(ListItemOrdinal::IsListItem(*this));

  int requested_value = 0;
  if (ParseHTMLInteger(value, requested_value))
    ordinal->SetExplicitValue(requested_value, *this);
  else
    ordinal->ClearExplicitValue(*this);
}

}  // namespace blink
